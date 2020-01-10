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


#ifndef Content_h
#define Content_h

#include "defines.h"
#include "XML.h"
#include "Props.h"
#include "counting_auto_ptr.h"



class Content;
class ContentTemplate;


class ContentParsed
{
 public:
  ContentParsed(const XMLObject& xml,
		counting_auto_ptr<Content>& content);
  virtual ~ContentParsed();

  Props props;
  counting_auto_ptr<Content> content;
  counting_auto_ptr<ContentTemplate> _replacement;

};


class Content
{
 public:
  virtual ~Content();

  String path() const;
  String state_ind() const;

  virtual XMLObject xml() const;

  const String type;

  Props _props;

  counting_auto_ptr<ContentTemplate> _replacement;

  virtual void add_replacement(const counting_auto_ptr<ContentTemplate>& replacement);
  std::list<counting_auto_ptr<ContentTemplate> > _avail_replacements;



  // internal
  virtual bool expandable(long long& max_size) const = 0;
  virtual bool shrinkable(long long& min_size) const = 0;


  // modifying fcns
  virtual void shrink(const String& path,
		      unsigned long long new_size,
		      const Props& new_props) = 0;
  virtual void expand(const String& path,
		      unsigned long long new_size,
		      const Props& new_props) = 0;

  virtual void apply_props_before_resize(const String& path,
					 unsigned long long old_size,
					 unsigned long long new_size,
					 const Props& new_props) = 0;
  virtual void apply_props_after_resize(const String& path,
					unsigned long long old_size,
					unsigned long long new_size,
					const Props& new_props) = 0;

  virtual bool removable() const;
  virtual void remove() = 0;


 protected:
  Content(const String& type, const String& path);

 private:


};


class ContentTemplate
{
 public:
  ContentTemplate(const XMLObject& xml,
		  counting_auto_ptr<Content>& content); // content to perform validation against
  virtual ~ContentTemplate();

  virtual XMLObject xml() const;

  Props _props;

  String type;

  std::map<String, String> attrs; // update if necessary

 protected:
  ContentTemplate(const String& type);


};


#endif  // Content_h
