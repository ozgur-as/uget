/*
 *
 *   Copyright (C) 2005-2020 by C.H. Huang
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

#include <stdint.h>
#include <string.h>
#include <UgStdio.h>
#include <UgtkUtil.h>
#include <gio/gio.h>

#if defined _WIN32 || defined _WIN64
#include <windows.h>
#include <shellapi.h>  // ShellExecuteW()
#endif

#if defined _WIN32 || defined _WIN64

gboolean  ugtk_launch_uri (const gchar* uri)
{
	// ShellExecuteA() returns an HINSTANCE; go through intptr_t so the
	// value is not truncated on 64-bit Windows.
	intptr_t  result;

	result = (intptr_t) ShellExecuteA(NULL, "open", uri, NULL, NULL, SW_SHOWNORMAL);
	if (result > 32)
		return TRUE;
	return FALSE;
}

gboolean  ugtk_launch_default_app (const gchar* folder, const gchar* file)
{
	gchar*     path;
	gunichar2* path_wcs;

	if (folder == NULL)
		path = g_build_filename (file, NULL);
	else
		path = g_build_filename (folder, file, NULL);
	if (g_file_test (path, G_FILE_TEST_EXISTS) == FALSE) {
		g_free (path);
		return FALSE;
	}

	// UNICODE
	path_wcs = g_utf8_to_utf16 (path, -1, NULL, NULL, NULL);
	g_free (path);
	ShellExecuteW (NULL, L"open", path_wcs, NULL, NULL, SW_SHOW);
	g_free (path_wcs);

	return TRUE;
}

#else
gboolean  ugtk_launch_uri (const gchar* uri)
{
	GError* error = NULL;

	g_app_info_launch_default_for_uri (uri, NULL, &error);
	if (error) {
		g_error_free (error);
		return FALSE;
	}
	return TRUE;
}

gboolean  ugtk_launch_default_app (const gchar* folder, const gchar* file)
{
	GError* error = NULL;
	GFile*  gfile;
	gchar*  uri;
	gchar*  path;
	gchar*  path_wcs;

	path = g_build_filename (folder, file, NULL);
	path_wcs = g_filename_from_utf8 (path, -1, NULL, NULL, NULL);
	g_free (path);
	if (g_file_test (path_wcs, G_FILE_TEST_EXISTS) == FALSE) {
		g_free (path_wcs);
		return FALSE;
	}

	gfile = g_file_new_for_path (path_wcs);
	g_free (path_wcs);
	uri = g_file_get_uri (gfile);
	g_object_unref (gfile);
	g_app_info_launch_default_for_uri (uri, NULL, &error);
	g_free (uri);

	if (error) {
#ifndef NDEBUG
		g_print ("%s", error->message);
#endif
		g_error_free (error);
	}

	return TRUE;
}

#endif // _WIN32 || _WIN64

// ----------------------------------------------------------------------------
// URI list functions
// get URIs from text file
GList*	ugtk_text_file_get_uris (const gchar* file_utf8, GError** error)
{
	GIOChannel*  channel;
	GList*       list;
	gchar*       string;
	gchar*       escaped;
	gsize        line_len;

	string = g_filename_from_utf8 (file_utf8, -1, NULL, NULL, NULL);
	channel = g_io_channel_new_file (string, "r", error);
	g_free (string);
	if (channel == NULL)
		return NULL;
	ugtk_io_channel_decide_encoding (channel);

	list = NULL;
	while (g_io_channel_read_line (channel, &string, NULL, &line_len, NULL) == G_IO_STATUS_NORMAL) {
		if (string == NULL)
			continue;
		string[line_len] = 0;		// clear '\n' in tail
		// check URI scheme
		escaped = g_uri_parse_scheme (string);
		if (escaped == NULL) {
			g_free (escaped);
			g_free (string);
		}
		else {
			g_free (escaped);
			// if URI is not valid UTF-8 string, escape it.
			if (g_utf8_validate (string, -1, NULL) == FALSE) {
				escaped = g_uri_escape_string (string,
						G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, FALSE);
				g_free (string);
				string = escaped;
			}
			list = g_list_prepend (list, string);
		}
	}
	g_io_channel_unref (channel);
	return g_list_reverse (list);
}

// get URIs from text
GList*	ugtk_text_get_uris (const gchar* text)
{
	GList* list;
	gchar* escaped;
	gchar* line;
	gint   line_len;
	gint   text_len;
	gint   offset;

	text_len = strlen (text);
	list = NULL;
	for (offset = 0;  offset < text_len;  offset += line_len +1) {
		line_len = strcspn (text + offset, "\r\n");
		line = g_strndup (text + offset, line_len);
		// check URI scheme
		escaped = g_uri_parse_scheme (line);
		if (escaped == NULL) {
			g_free (escaped);
			g_free (line);
		}
		else {
			g_free (escaped);
			// if URI is not valid UTF-8 string, escape it.
			if (g_utf8_validate (line, -1, NULL) == FALSE) {
				escaped = g_uri_escape_string (line,
						G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, FALSE);
				g_free (line);
				line = escaped;
			}
			list = g_list_prepend (list, line);
		}
	}

	return g_list_reverse (list);
}

GList*	ugtk_uri_list_remove_scheme (GList* list, const gchar* scheme)
{
	GList*  link;
	gchar*  text;

	for (link = list;  link;  link = link->next) {
		text = g_uri_parse_scheme (link->data);
		if (text == NULL || strcmp (text, scheme) == 0) {
			g_free (link->data);
			link->data = NULL;
		}
		g_free (text);
	}
	return g_list_remove_all (list, NULL);
}

// ----------------------------------------------------------------------------
// Used by ug_io_channel_decide_encoding()
// BOM = Byte Order Mark
#define UG_BOM_UTF32BE          "\x00\x00\xFE\xFF"
#define UG_BOM_UTF32BE_LEN      4
#define UG_BOM_UTF32LE          "\xFF\xFE\x00\x00"
#define UG_BOM_UTF32LE_LEN      4
#define UG_BOM_UTF8             "\xEF\xBB\xBF"
#define	UG_BOM_UTF8_LEN         3
#define UG_BOM_UTF16BE          "\xFE\xFF"
#define	UG_BOM_UTF16BE_LEN      2
#define UG_BOM_UTF16LE          "\xFF\xFE"
#define	UG_BOM_UTF16LE_LEN      2

const char* ugtk_io_channel_decide_encoding (GIOChannel* channel)
{
	gchar*  encoding;
	gchar   bom[4];
	guint   bom_len;

	// The internal encoding is always UTF-8.
	// set encoding NULL is safe to use with binary data.
	g_io_channel_set_encoding (channel, NULL, NULL);
	// read 4 bytes BOM (Byte Order Mark)
	if (g_io_channel_read_chars (channel, bom, 4, NULL, NULL) != G_IO_STATUS_NORMAL)
		return NULL;

	if (memcmp (bom, UG_BOM_UTF32BE, UG_BOM_UTF32BE_LEN) == 0) {
		bom_len = UG_BOM_UTF32BE_LEN;
		encoding = "UTF-32BE";
	}
	else if (memcmp (bom, UG_BOM_UTF32LE, UG_BOM_UTF32LE_LEN) == 0) {
		bom_len = UG_BOM_UTF32LE_LEN;
		encoding = "UTF-32LE";
	}
	else if (memcmp (bom, UG_BOM_UTF8, UG_BOM_UTF8_LEN) == 0) {
		bom_len = UG_BOM_UTF8_LEN;
		encoding = "UTF-8";
	}
	else if (memcmp (bom, UG_BOM_UTF16BE, UG_BOM_UTF16BE_LEN) == 0) {
		bom_len = UG_BOM_UTF16BE_LEN;
		encoding = "UTF-16BE";
	}
	else if (memcmp (bom, UG_BOM_UTF16LE, UG_BOM_UTF16LE_LEN) == 0) {
		bom_len = UG_BOM_UTF16LE_LEN;
		encoding = "UTF-16LE";
	}
	else {
		bom_len = 0;
		encoding = NULL;
//		encoding = "UTF-8";
	}
	// repositioned before set encoding. This flushes all the internal buffers.
	g_io_channel_seek_position (channel, bom_len, G_SEEK_SET, NULL);
	// The encoding can be set now.
	g_io_channel_set_encoding (channel, encoding, NULL);
	return encoding;
}

