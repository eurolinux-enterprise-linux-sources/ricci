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

#ifndef __CONGA_FILE_H
#define __CONGA_FILE_H

#include "String.h"
#include "counting_auto_ptr.h"

class File_pimpl
{
	public:
		File_pimpl(void*, bool&);
		virtual ~File_pimpl();
		void* const fs;
	private:
		File_pimpl(const File_pimpl&);
		File_pimpl& operator=(const File_pimpl&);
};


class File
{
	public:
		// throw if non-existent
		static File open(const String& filepath, bool rw=false);
		// same as open, but create if nonexistent
		static File create(const String& filepath, bool truncate=false);
		virtual ~File();

		String path() const;
		long size() const;

		String read() const; // return content

		File& append(const String& data); // append data to the end of file

		// replace content with data, return old content
		String replace(const String& data);

		void shred();
		void unlink();

		operator const String () const;

	private:
		File(counting_auto_ptr<File_pimpl>, const String& path, bool writable);
		counting_auto_ptr<Mutex> _mutex;
		counting_auto_ptr<File_pimpl> _pimpl;
		const String _path;
		const bool _writable;

		void check_failed() const;
};

#endif
