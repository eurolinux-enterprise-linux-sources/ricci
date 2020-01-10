/*
  Copyright Red Hat, Inc. 2005

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2, or (at your option) any
  later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to the
  Free Software Foundation, Inc.,  675 Mass Ave, Cambridge,
  MA 02139, USA.
*/
/*
 * Author: Stanko Kupcevic <kupcevic@redhat.com>
 */


#include "LV.h"
#include "LVM.h"
#include "BDFactory.h"
#include "FSController.h"
#include "defines.h"
#include "utils.h"
#include "ContentFactory.h"
#include "ContentNone.h"
#include "LVMClusterLockingError.h"


using namespace std;



counting_auto_ptr<BD>
create_LV(const BDTemplate& bd_temp)
{
  // everything is already validated :)

  String vgname = bd_temp.props.get("vgname").get_string();


  // if VG is marked as clustered, but cluster locking is not available, throw
  if (!LVM::clustered_enabled() &&
      bd_temp.props.get("clustered").get_bool())
    throw LVMClusterLockingError();


  String lvname = bd_temp.props.get("lvname").get_string();
  long long size = bd_temp.props.get("size").get_int();

  bool snapshot = bd_temp.props.get("snapshot").get_bool();
  if (snapshot) {
    String origin = bd_temp.props.get("snapshot_origin").get_string();
    String origin_path = String("/dev/") + vgname + "/" + origin;
    LVM::lvcreate_snap(lvname, origin_path, size);
  } else
    LVM::lvcreate(vgname, lvname, size);

  utils::clear_cache();

  counting_auto_ptr<BD> bd = BDFactory::get_bd(String("/dev/") + vgname + "/" + lvname);
  if (!snapshot)
    ContentFactory().create_content(bd, bd_temp.content_parsed);

  utils::clear_cache();

  return BDFactory::get_bd(String("/dev/") + vgname + "/" + lvname);
}





// ##### LV #####

LV::LV(const String& path) :
  BD(MAPPER_VG_TYPE,
     VG_PREFIX + LVM::vgname_from_lvpath(path),
     BD_LV_TYPE,
     path)
{
  LVM::probe_lv(_path, _props);

  if (content->removable()) {
    removable(true);
    if (content->type == CONTENT_NONE_TYPE) {
      list<counting_auto_ptr<ContentTemplate> > fss = FSController().get_available_fss();
      for (list<counting_auto_ptr<ContentTemplate> >::const_iterator iter = fss.begin();
	   iter != fss.end();
	   iter++) {
	content->add_replacement(*iter);
      }
    } else
      content->add_replacement(counting_auto_ptr<ContentTemplate>(new ContentNoneTemplate()));
  }

  // LVs under snapshots can be neither removed nor resized
  if (_props.get("snapshots").get_list_str().size()) {
    removable(false);

    long long extent_size = _props.get("extent_size").get_int();
    long long size = _props.get("size").get_int();
    _props.set(Variable("size",
			size,
			size,
			size,
			extent_size));
  }

  // snapshots should neither replace nor resize content
  if (_props.get("snapshot").get_bool()) {
    content->_avail_replacements.clear();

    // adjust size based on snap_percent
    long long usage = _props.get("snapshot_usage_percent").get_int();
    const Variable size_var(_props.get("size"));
    const XMLObject xml(size_var.xml());
    long long size = size_var.get_int();
    long long min  = utils::to_long(xml.get_attr("min"));
    long long max  = utils::to_long(xml.get_attr("max"));
    long long step = utils::to_long(xml.get_attr("step"));
    long long min_size = (long long) (size * usage) / 100;
    if (min_size > min)
      min = min_size;
    else if (min_size > max)
      min = max;
    if (min_size == max)
      _props.set(Variable("size", size));
    else
      _props.set(Variable("size", size, min, max, step));
  } else {
    // adjust size based on content
    const Variable size_var(_props.get("size"));
    const XMLObject xml(size_var.xml());
    long long size = size_var.get_int();
    long long min  = utils::to_long(xml.get_attr("min"));
    long long max  = utils::to_long(xml.get_attr("max"));
    long long step = utils::to_long(xml.get_attr("step"));
    long long max_con_size;
    if (content->expandable(max_con_size))
      max = (max_con_size > max) ? max : max_con_size;
    else
      max = size;
    long long min_con_size;
    if (content->shrinkable(min_con_size))
      min = (min_con_size < min) ? min : min_con_size;
    else
      min = size;
    if (min == max)
      _props.set(Variable("size", size));
    else
      _props.set(Variable("size", size, min, max, step));
  }
}

LV::~LV()
{}



String
LV::state_ind() const
{
  Props tmp_props(_props);

  // remove "snapshot_usage_percent" as its change shouldn't affect state_ind
  tmp_props.set(Variable("snapshot_usage_percent", (long long) 0));

  XMLObject t;
  t.add_child(tmp_props.xml());
  t.add_child(content->xml());

  return utils::hash_str(generateXML(t) + path() + _mapper_id);
}

counting_auto_ptr<BD>
LV::apply(const BDParsed& bd_parsed)
{
  // if VG is marked as clustered, but cluster locking is not available, throw
  if (_props.get("clustered").get_bool() &&
      !LVM::clustered_enabled())
    throw LVMClusterLockingError();

  // snapshots neither resize nor replace content, see LV()

  if (_props.get("snapshot").get_bool()) {
    long long orig_size = this->size();
    long long new_size = bd_parsed.props.get("size").get_int();

    // first
    this->apply_props_before_resize(bd_parsed.props);
    content->apply_props_before_resize(path(),
				       orig_size,
				       orig_size,
				       bd_parsed.content->props);

    // second - size
    if (orig_size > new_size) {
      this->shrink(new_size, bd_parsed.props);
    } else if (orig_size < new_size) {
      this->expand(new_size, bd_parsed.props);
    }

    // third
    this->apply_props_after_resize(bd_parsed.props);
    content->apply_props_after_resize(path(),
				      orig_size,
				      orig_size,
				      bd_parsed.content->props);

    utils::clear_cache();
    return BDFactory().get_bd(path());
  } else
    return this->BD::apply(bd_parsed);
}

void
LV::shrink(unsigned long long new_size,
	   const Props& new_props)
{
  LVM::lvreduce(_path, new_size);
}
void
LV::expand(unsigned long long new_size,
	   const Props& new_props)
{
  LVM::lvextend(_path, new_size);
}
String
LV::apply_props_before_resize(const Props& new_props)
{
  return path();
}
String
LV::apply_props_after_resize(const Props& new_props)
{
  return path();
}

void
LV::remove()
{
  // if VG is marked as clustered, but cluster locking is not available, throw
	bool lv_clustered = _props.get("clustered").get_bool();
	if (lv_clustered && !LVM::clustered_enabled())
		throw LVMClusterLockingError();

	/* Don't hose volumes that may be mounted elsewhere */
	if (!lv_clustered)
		content->remove();
	LVM::lvremove(path());
}




// ##### LVTemplate #####

LVTemplate::LVTemplate(const String& mapper_id,
		       const String& mapper_state_ind) :
  BDTemplate(MAPPER_VG_TYPE,
	     mapper_id,
	     mapper_state_ind,
	     BD_LV_TYPE)
{
  if (content->type == CONTENT_NONE_TYPE) {
    list<counting_auto_ptr<ContentTemplate> > fss = FSController().get_available_fss();
    for (list<counting_auto_ptr<ContentTemplate> >::const_iterator iter = fss.begin();
	 iter != fss.end();
	 iter++) {
      content->add_replacement(*iter);
    }
  }
}

LVTemplate::~LVTemplate()
{}
