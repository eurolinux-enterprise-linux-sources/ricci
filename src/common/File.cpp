/*
** Copyright (C) Red Hat, Inc. 2006-2009
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License version 2 as
** published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; see the file COPYING. If not, write to the
** Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
** MA 02139, USA.
*/

/*
 * Author: Stanko Kupcevic <kupcevic@redhat.com>
 */

#include "File.h"
#include <fstream>
#include <fcntl.h>
#include <errno.h>

using namespace std;

File_pimpl::File_pimpl(void *fs, bool& owner) :
	fs(fs)
{
	if (fs == NULL)
		throw String("fs_ptr is null");
	owner = true;
}

File_pimpl::~File_pimpl()
{
	fstream *ptr = (fstream *) fs;
	delete ptr;
}

File
File::open(const String& filepath, bool rw)
{
	if (access(filepath.c_str(), R_OK))
		throw String("unable to read file ") + filepath;

	ios_base::openmode mode = ios_base::in;
	if (rw)
		mode |= ios_base::out;

	counting_auto_ptr<File_pimpl> pimpl;
	bool ownership_taken = false;
	fstream *fs = new fstream(filepath.c_str(), mode);
	try {
		pimpl = counting_auto_ptr<File_pimpl>(new File_pimpl(fs, ownership_taken));
	} catch ( ... ) {
		if (!ownership_taken)
			delete fs;
		throw;
	}
	return File(pimpl, filepath, rw);
}

File
File::create(const String& filepath, bool truncate)
{
	int t = ::open(filepath.c_str(), O_CREAT | O_RDWR, 0640);
	if (t != -1) {
		while (close(t) && errno == EINTR)
			;
	}

	ios_base::openmode mode = ios_base::in;
	mode |= ios_base::out;
	if (truncate)
		mode |= ios_base::trunc;

	counting_auto_ptr<File_pimpl> pimpl;
	bool ownership_taken = false;
	fstream *fs = new fstream(filepath.c_str(), mode);

	try {
		pimpl = counting_auto_ptr<File_pimpl>(new File_pimpl(fs, ownership_taken));
	} catch ( ... ) {
		if (!ownership_taken)
			delete fs;
		throw;
	}
	return File(pimpl, filepath, true);
}

File::File(	counting_auto_ptr<File_pimpl> pimpl,
			const String& path,
			bool writable) :
	_mutex(counting_auto_ptr<Mutex>(new Mutex())),
	_pimpl(pimpl),
	_path(path),
	_writable(writable)
{
	if (!((fstream *) _pimpl->fs)->is_open())
		throw String("unable to open ") + _path;
	check_failed();
}

File::~File()
{
	if (_writable)
		((fstream*) _pimpl->fs)->flush();
}

String
File::path() const
{
	MutexLocker l(*_mutex);
	return _path;
}

long
File::size() const
{
	MutexLocker l(*_mutex);
	((fstream *) _pimpl->fs)->seekg(0, ios::end);
	check_failed();

	long s = ((fstream *) _pimpl->fs)->tellg();
	check_failed();

	if (s < 0)
		throw String("size of file ") + _path + " is negative";
	return s;
}

String
File::read() const
{
	MutexLocker l(*_mutex);

	long len = size();
	char buff[len];
	try {
		((fstream *) _pimpl->fs)->seekg(0, ios::beg);
		check_failed();
		((fstream *) _pimpl->fs)->read(buff, len);
		check_failed();
		String ret(buff, len);
		::shred(buff, len);
		return ret;
	} catch ( ... ) {
		::shred(buff, len);
		throw;
	}
}

File&
File::append(const String& data)
{
	MutexLocker l(*_mutex);
	if (!_writable)
		throw String("not writable");

	((fstream *) _pimpl->fs)->seekp(0, ios::end);
	check_failed();

	((fstream *) _pimpl->fs)->write(data.c_str(), data.size());
	check_failed();

	((fstream *) _pimpl->fs)->flush();
	check_failed();
	return *this;
}

String
File::replace(const String& data)
{
	MutexLocker l(*_mutex);
	if (!_writable)
		throw String("not writable");

	String old(read());
	create(_path, true);
	append(data);
	return old;
}

void
File::shred()
{
	MutexLocker l(*_mutex);
	if (!_writable)
		throw String("not writable");

	unsigned int len = size();
	((fstream *) _pimpl->fs)->seekp(0, ios::beg);
	check_failed();

	// should use random source (paranoid)
	// doesn't work on journaled fss anyways
	((fstream *) _pimpl->fs)->write(String(len, 'o').c_str(), len);
	check_failed();
}

void
File::unlink()
{
	MutexLocker l(*_mutex);
	if (::unlink(_path.c_str()))
		throw String("unlink failed: " + String(strerror(errno)));
}

File::operator const String () const
{
	return read();
}

void
File::check_failed() const
{
	if (((fstream *) _pimpl->fs)->fail())
		throw String("IO error");
}
