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

#include <stdlib.h>
#include <strings.h>
#include <UgString.h>
#include <UgUri.h>
#include <UgUtil.h>

// ----------------------------------------------------------------------------
// UgUri

int  ug_uri_init (UgUri* upart, const char* uri)
{
	const char* cur;
	const char* tmp;

	// scheme - make sure ':' before '/', '%', '?', and '#'
#if defined _WIN32 || defined _WIN64
	cur = strpbrk (uri, ":\\/%?#");
#else
	cur = strpbrk (uri, ":/%?#");
#endif
	if (cur && cur[0] == ':') {
		if (upart == NULL)
			return cur - uri;
		upart->scheme_len = cur - uri;
		cur++;
	}
	else {
		if (upart == NULL)
			return 0;
		upart->scheme_len = 0;
		cur = uri;
	}

	upart->uri = uri;

#if defined _WIN32 || defined _WIN64
	// Windows Path
	if (upart->scheme_len == 1) {
		upart->scheme_len = 0;
		cur = uri;
	}
#endif // _WIN32 || _WIN64

	// authority & path
	if (upart->scheme_len && cur[0] == '/' && cur[1] == '/') {
		cur += 2;
		upart->authority = cur - uri;
		cur += strcspn (cur, "/");
	}
	else
		upart->authority = -1;
	upart->path = cur - uri;

	// file
	upart->file = -1;
	if (cur[0]) {
		for (; ; ) {
#if defined _WIN32 || defined _WIN64
			tmp = strpbrk (cur, "\\/?#");
			if (tmp == NULL || (tmp[0] != '/' && tmp[0] != '\\')) {
#else
			tmp = strpbrk (cur, "/?#");
			if (tmp == NULL || tmp[0] != '/') {
#endif // _WIN32 || _WIN64
				upart->file = cur - uri;
				break;
			}
			cur = tmp + 1;
		}
	}

	// query
	if ((tmp = strchr (cur, '?')) == NULL)
		upart->query = -1;
	else {
		cur = tmp + 1;
		upart->query = cur - uri;
	}

	// fragment
	if ((tmp = strrchr (cur, '#')) == NULL)
		upart->fragment = -1;
	else {
		cur = tmp + 1;
		if (cur - uri <= INT16_MAX)
			upart->fragment = cur - uri;
		else
			upart->fragment = -1;
	}

	// host & port
	upart->port = -1;
	if (upart->authority == -1)
		upart->host = -1;
	else {
		upart->host = upart->authority;
		tmp = uri + upart->authority;
		for (cur = uri + upart->path -1;  cur >= tmp;  cur--) {
			if (cur[0] == '@') {
				upart->host = cur - uri + 1;
				break;
			}
			if (cur[0] == ':')
				upart->port = cur - uri + 1;
		}
	}

	return upart->scheme_len;
}

int  ug_uri_part_scheme (UgUri* uuri, const char** scheme)
{
	if (scheme && uuri->scheme_len)
		*scheme = uuri->uri;
	return uuri->scheme_len;
}

int  ug_uri_part_file (UgUri* uuri, const char** file)
{
	if (uuri->file != -1) {
		if (file)
			*file = uuri->uri + uuri->file;
		if (uuri->query != -1)
			return uuri->query - uuri->file - 1;   // - '?'
		if (uuri->fragment != -1)
			return uuri->fragment - uuri->file - 1;  // - '#'
		return strlen (uuri->uri + uuri->file);
	}
	return 0;
}

int  ug_uri_part_file_ext (UgUri* uuri, const char** ext)
{
	const char* beg;
	const char* end;
	int  len;

	if (uuri->file != -1) {
		len = ug_uri_part_file (uuri, &beg);
		end = uuri->uri + uuri->file + len -1;
		for (;  end >= beg;  end--) {
			if (end[0] == '.') {
				end += 1;	// + '.'
				if (ext)
					*ext = end;
				return len - (end - beg);
			}
		}
	}
	return 0;
}

int  ug_uri_part_query (UgUri* uuri, const char** query)
{
	if (uuri->query != -1) {
		if (query)
			*query = uuri->uri + uuri->query;
		if (uuri->fragment != -1)
			return uuri->fragment - uuri->query -1;  // - '#'
		return strlen (uuri->uri + uuri->query);
	}
	return 0;
}

int  ug_uri_part_fragment (UgUri* uuri, const char** fragment)
{
	if (uuri->fragment != -1) {
		if (fragment)
			*fragment = uuri->uri + uuri->fragment;
		return strlen (uuri->uri + uuri->fragment);
	}
	return 0;
}

int  ug_uri_part_referrer (UgUri* uuri, const char** referrer)
{
	if (referrer)
		*referrer = uuri->uri;
	if (uuri->file == -1)
		return uuri->path;
	return uuri->file;
}

int  ug_uri_part_user (UgUri* uuri, const char** user)
{
	const char* beg;
	const char* end;

	if (uuri->authority == uuri->host)
		return 0;

	beg = uuri->uri + uuri->authority;
	end = uuri->uri + uuri->host - 1;    // - '@'
	if (user)
		*user = beg;
	for (; beg < end;  beg++) {
		if (beg[0] == ':')
			break;
	}
	return beg - uuri->uri - uuri->authority;
}

int  ug_uri_part_password (UgUri* uuri, const char** password)
{
	const char* tmp;
	int  length;

	length = ug_uri_part_user (uuri, &tmp);
	if (length && tmp[length] == ':') {
		tmp += length + 1;  // + ':'
		if (password)
			*password = tmp;
		return uuri->host - (tmp - uuri->uri) - 1;  // - '@'
	}
	return 0;
}

int  ug_uri_part_host (UgUri* uuri, const char** host)
{
	if (uuri->host != -1) {
		if (host)
			*host = uuri->uri + uuri->host;
		if (uuri->port != -1)
			return uuri->port - uuri->host - 1;   // - ':'
		else
			return uuri->path - uuri->host;
	}
	return 0;
}

int  ug_uri_part_port (UgUri* uuri, const char** port)
{
	if (uuri->port != -1) {
		if (port)
			*port = uuri->uri + uuri->port;
		return uuri->path - uuri->port;
	}
	return 0;
}

int  ug_uri_get_port (UgUri* uuri)
{
	if (uuri->port != -1)
		return strtol (uuri->uri + uuri->port, NULL, 10);
	return -1;
}

char* ug_uri_get_file (UgUri* uuri)
{
	const char*	str;
	char*		name;
	int			len;

	len = ug_uri_part_file (uuri, &str);
	if (len == 0)
		return NULL;
	name = ug_malloc (len + 1);
	ug_decode_uri (str, len, name);
	if (ug_utf8_get_invalid (name, NULL) == -1)
		return name;

	ug_free (name);
	return ug_strndup (str, len);
}

int  ug_uri_is_file(UgUri* uuri)
{
	if (uuri->scheme_len == 4) {
		if (strncmp(uuri->uri, "file", 4) != 0)
			return FALSE;
	}
	else if (uuri->scheme_len != 0)
		return FALSE;
	return TRUE;
}

// ------------------------------------
// match uri functions

int  ug_uri_match_hosts (UgUri* uuri, char** hosts)
{
	const char* str;
	const char* end1;
	const char* end2;
	int         len;
	int         lenHost;
	int         nthHost;

	len = ug_uri_part_host (uuri, &str);
	if (len) {
		for (nthHost = 0;  *hosts;  hosts++, nthHost++) {
			lenHost = strlen (*hosts);
			if (lenHost > len)
				continue;
			// compare host from head
			if (strncasecmp (uuri->uri, *hosts, len) == 0)
				return nthHost;
			// compare host from tail
			end1 = str + len -1;
			end2 = *hosts + lenHost -1;
			for (;  lenHost > 0;  lenHost--, end1--, end2--) {
				if (end1[0] != end2[0])
					break;
			}

			if (lenHost == 0)
				return nthHost;
		}
	}
	return -1;
}

int  ug_uri_match_schemes (UgUri* uuri, char** schemes)
{
	int         len;
	int         index;

	len = ug_uri_part_scheme (uuri, NULL);
	if (len) {
		for (index = 0;  *schemes;  schemes++, index++) {
			if (strncasecmp (uuri->uri, *schemes, len) == 0)
				return index;
		}
	}
	return -1;
}

int  ug_uri_match_file_exts (UgUri* uuri, char** exts)
{
	const char* str;
	int         len;
	int         index;

	len = ug_uri_part_file_ext (uuri, &str);
	if (len) {
		for (index = 0;  *exts;  exts++, index++) {
			if (strncasecmp (str, *exts, len) == 0)
				return index;
		}
	}
	return -1;
}

// ------------------------------------
// UgUriQuery

int  ug_uri_query_part (UgUriQuery* uuquery, const char* query_field)
{
	int  ret_value;

	if (query_field == NULL)
		return 0;
	uuquery->field_len = strcspn (query_field, "=&#;,");
	if (uuquery->field_len == 0)
		return 0;

	query_field += uuquery->field_len;
	if (query_field[0] == '=') {
		uuquery->value = (char*) query_field + 1;   // + '='
		uuquery->value_len = strcspn (uuquery->value, "&#;,");
		query_field = uuquery->value + uuquery->value_len;
		ret_value = 2;
	}
	else {
		uuquery->value = NULL;
		uuquery->value_len = 0;
		ret_value = 1;
	}

	// next field or next value
	switch (query_field[0]) {
	case ',':
		uuquery->value_next = (char*) query_field + 1;
		uuquery->field_next = NULL;
		break;

	case '&':
	case ';':
		uuquery->value_next = NULL;
		uuquery->field_next = (char*) query_field + 1;
		break;

	default:
		uuquery->value_next = NULL;
		uuquery->field_next = NULL;
		break;
	}

	return ret_value;
}

// ------------------------------------
// Other URI functions

char* ug_filename_from_uri (const char* str)
{
	UgUri  uuri;

	ug_uri_init (&uuri, str);
	return ug_uri_get_file ((UgUri*) &uuri);
}

// return length of decoded uri. param dest can be param uri or NULL.
int   ug_decode_uri (const char* uri, int length, char* dest)
{
	const char*  uri_end;
	char*  dest_beg;
	char*  endptr;
	char   buf[3];

	if (length == -1)
		length = strlen (uri);
	dest_beg = dest;

	for (uri_end = uri + length;  uri < uri_end;  uri++, dest++) {
		if (uri[0] == '%' && uri + 2 < uri_end) {
			buf[0] = uri[1];
			buf[1] = uri[2];
			buf[2] = 0;
			endptr = NULL;
			*(uint8_t*)buf = (uint8_t) strtoul (buf, &endptr, 16);
			if (endptr == buf + 2) {
				if (dest_beg)
					*dest = *buf;
				uri += 2;
				continue;
			}
		}

		if (dest_beg) {
			if (uri[0] == '+')
				*dest = ' ';
			else
				*dest = *uri;
		}
	}

	if (dest_beg)
		*dest = 0;
	return dest - dest_beg;
}
