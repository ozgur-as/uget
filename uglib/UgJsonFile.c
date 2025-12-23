/*
 *
 *   Copyright (C) 2012-2020 by C.H. Huang
 *   plushuang.tw@gmail.com
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  ---
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of portions of this program with the
 *  OpenSSL library under certain conditions as described in each
 *  individual source file, and distribute linked combinations
 *  including the two.
 *  You must obey the GNU Lesser General Public License in all respects
 *  for all of the code used other than OpenSSL.  If you modify
 *  file(s) with this exception, you may extend this exception to your
 *  version of the file(s), but you are not obligated to do so.  If you
 *  do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source
 *  files in the program, then also delete it here.
 *
 */

#include <UgStdio.h>
#include <UgDefine.h>
#include <UgJsonFile.h>

static int  buffer_to_fd (UgBuffer* buffer);

UgJsonFile*  ug_json_file_new (int buffer_size)
{
	UgJsonFile*  jfile;

	if (buffer_size == 0)
		buffer_size = 4096;

	if (buffer_size == 0)
		jfile = ug_malloc (sizeof (UgJsonFile));
	else
		jfile = ug_malloc (sizeof (UgJsonFile) + sizeof (char) * buffer_size - 1);

	jfile->fd = -1;
	jfile->n_bytes = buffer_size;
	ug_json_init (&jfile->json);
	return jfile;
}

void  ug_json_file_free (UgJsonFile* jfile)
{
	if (jfile->fd != -1)
		ug_close (jfile->fd);
	ug_json_final (&jfile->json);
	ug_free (jfile);
}

int   ug_json_file_begin_parse (UgJsonFile* jfile, const char* path)
{
	int  fd;

//	fd = open (path, O_RDONLY, 0);
	fd = ug_open (path, UG_O_RDONLY | UG_O_TEXT, 0);
	if (fd == -1)
		return FALSE;

	return ug_json_file_begin_parse_fd (jfile, fd);
}

int   ug_json_file_begin_write (UgJsonFile* jfile, const char* path, UgJsonFormat format)
{
	int  fd;

//	fd = open (path, O_CREAT | O_WRONLY | O_TRUNC,
//			S_IREAD | S_IWRITE | S_IRGRP | S_IROTH);
	fd = ug_open (path, UG_O_CREAT | UG_O_WRONLY | UG_O_TRUNC | UG_O_TEXT,
			UG_S_IREAD | UG_S_IWRITE | UG_S_IRGRP | UG_S_IROTH);
	if (fd == -1)
		return FALSE;

	return ug_json_file_begin_write_fd (jfile, fd, format);
}

int   ug_json_file_begin_parse_fd (UgJsonFile* jfile, int fd)
{
	jfile->fd = fd;
	// ready to parse
	ug_json_begin_parse (&jfile->json);
	return TRUE;
}

int   ug_json_file_begin_write_fd (UgJsonFile* jfile, int fd, UgJsonFormat format)
{
	jfile->fd = fd;
	// init UgBuffer for writer
	ug_buffer_init_external (&jfile->buffer, jfile->bytes, jfile->n_bytes);
	jfile->buffer.data = (void*)(uintptr_t) jfile->fd;
	jfile->buffer.more = buffer_to_fd;
	// ready to write
	ug_json_begin_write (&jfile->json, format, &jfile->buffer);
	return TRUE;
}

UgJsonError   ug_json_file_end_parse (UgJsonFile* jfile)
{
	UgJsonError  error;
	int          len;

	do {
		len = ug_read (jfile->fd, &jfile->bytes, jfile->n_bytes);
		error = ug_json_parse (&jfile->json, jfile->bytes, len);
		if (error < 0)
			goto exit;
	} while (len > 0);

	error = ug_json_end_parse (&jfile->json);
exit:
	ug_close (jfile->fd);
	jfile->fd = -1;
	return error;
}

void  ug_json_file_end_write (UgJsonFile* jfile)
{
	ug_json_end_write (&jfile->json);
	ug_buffer_clear (&jfile->buffer, FALSE);
	if (ug_write (jfile->fd, "\n\n", 2)) {}

	// close() doesn't call fsync()
	// If you want to avoid delayed write, call fsync() before close()
	ug_sync (jfile->fd);

	ug_close (jfile->fd);
	jfile->fd = -1;
}

// ----------------------------------------------------------------------------
// static function for ug_json_file_save

// UgBufferFunc
static int  buffer_to_fd (UgBuffer* buffer)
{
	int     fd;

	fd = (uintptr_t)buffer->data;
	if (ug_write (fd, buffer->beg, ug_buffer_length (buffer))) {}
	buffer->cur = buffer->beg;
	return 0;
}


