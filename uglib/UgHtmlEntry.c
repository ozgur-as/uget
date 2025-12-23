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

#include <time.h>
#include <stdlib.h>
#include <strings.h>
#include <UgString.h>
#include <UgHtmlEntry.h>

#if defined(_MSC_VER)
#define strtoll     _strtoi64
#define strtoull    _strtoui64
#endif


// ----------------------------------------------------------------------------
// unknown
void ug_html_start_element_unknown (UgHtml*        uhtml,
                                    const char*    element_name,
                                    const char**   attribute_names,
                                    const char**   attribute_values,
                                    void*          dest,
                                    void*          data)
{
	ug_html_push (uhtml, &ug_html_parser_unknown, NULL, NULL);
}

const UgHtmlParser   ug_html_parser_unknown =
{
	(void*) ug_html_start_element_unknown,
	(void*) ug_html_pop,
	(void*) NULL,
};

// ----------------------------------------------------------------------------
// time_t

static void entry_text_time_rfc822 (UgHtml*        uhtml,
                                    const char*    text,
                                    int            text_len,
                                    void*          dest,
                                    void*          data)
{
	*(time_t*) dest = ug_str_rfc822_to_time (text);
}

const UgHtmlParser   ug_html_parser_time_rfc822 =
{
	(void*) ug_html_start_element_unknown,
	(void*) ug_html_pop,
	(void*) entry_text_time_rfc822,
};

static void entry_text_time_rfc3339 (UgHtml*        uhtml,
                                     const char*    text,
                                     int            text_len,
                                     void*          dest,
                                     void*          data)
{
	*(time_t*) dest = ug_str_rfc3339_to_time (text);
}

const UgHtmlParser   ug_html_parser_time_rfc3339 =
{
	(void*) ug_html_start_element_unknown,
	(void*) ug_html_pop,
	(void*) entry_text_time_rfc3339,
};

// ----------------------------------------------------------------------------
// bool

static void entry_text_bool (UgHtml*        uhtml,
                             const char*    text,
                             int            text_len,
                             void*          dest,
                             void*          data)
{
	if (strncasecmp (text, "false", text_len) == 0 || text[0] == '0')
		*(int*) dest = 0;
	else
		*(int*) dest = 1;
}

const UgHtmlParser  ug_html_parser_bool =
{
	(void*) ug_html_start_element_unknown,
	(void*) ug_html_pop,
	(void*) entry_text_bool,
};

// ----------------------------------------------------------------------------
// int

static void entry_text_int (UgHtml*        uhtml,
                            const char*    text,
                            int            text_len,
                            void*          dest,
                            void*          data)
{
	*(int*) dest = strtol (text, NULL, 10);
}

const UgHtmlParser  ug_html_parser_int =
{
	(void*) ug_html_start_element_unknown,
	(void*) ug_html_pop,
	(void*) entry_text_int,
};

// ----------------------------------------------------------------------------
// unsigned int

static void entry_text_uint (UgHtml*        uhtml,
                             const char*    text,
                             int            text_len,
                             void*          dest,
                             void*          data)
{
	*(unsigned int*) dest = (unsigned int) strtoul (text, NULL, 10);
}

const UgHtmlParser  ug_html_parser_uint =
{
	(void*) ug_html_start_element_unknown,
	(void*) ug_html_pop,
	(void*) entry_text_uint,
};

// ----------------------------------------------------------------------------
// int64_t

static void entry_text_int64 (UgHtml*        uhtml,
                              const char*    text,
                              int            text_len,
                              void*          dest,
                              void*          data)
{
	// C99 Standard
	*(int64_t*) dest = strtoll (text, NULL, 10);
}

const UgHtmlParser  ug_html_parser_int64 =
{
	(void*) ug_html_start_element_unknown,
	(void*) ug_html_pop,
	(void*) entry_text_int64,
};

// ----------------------------------------------------------------------------
// uint64_t

static void entry_text_uint64 (UgHtml*        uhtml,
                               const char*    text,
                               int            text_len,
                               void*          dest,
                               void*          data)
{
	// C99 Standard
	*(uint64_t*) dest = strtoull (text, NULL, 10);
}

const UgHtmlParser  ug_html_parser_uint64 =
{
	(void*) ug_html_start_element_unknown,
	(void*) ug_html_pop,
	(void*) entry_text_uint64,
};

// ----------------------------------------------------------------------------
// double

static void entry_text_double (UgHtml*        uhtml,
                               const char*    text,
                               int            text_len,
                               void*          dest,
                               void*          data)
{
	*(double*) dest = strtod (text, NULL);
}

const UgHtmlParser  ug_html_parser_double =
{
	(void*) ug_html_start_element_unknown,
	(void*) ug_html_pop,
	(void*) entry_text_double,
};

// ----------------------------------------------------------------------------
// string

static void entry_text_string (UgHtml*        uhtml,
                               const char*    text,
                               int            text_len,
                               void*          dest,
                               void*          data)
{
	*(char**) dest = ug_strdup (text);
}

const UgHtmlParser  ug_html_parser_string =
{
	(void*) ug_html_start_element_unknown,
	(void*) ug_html_pop,
	(void*) entry_text_string,
};

// ----------------------------------------------------------------------------
// custom

const UgHtmlParser  ug_html_parser_custom =
{
	(void*) NULL,
	(void*) NULL,
	(void*) NULL,
};

// ----------------------------------------------------------------------------

void ug_html_start_element_entry (UgHtml*        uhtml,
                                  const char*    element_name,
                                  const char**   attribute_names,
                                  const char**   attribute_values,
                                  void*          dest,
                                  void*          entry0)
{
	const UgHtmlEntry*  entry;
	const UgHtmlParser* parser;

	for (entry = (UgHtmlEntry*) entry0;  entry->parser;  entry++) {
		if (entry->name && strcmp (entry->name, uhtml->element_name) != 0)
			continue;
		// get destination
		dest = ((char*) dest) + entry->offset;

		parser = entry->parser;
		if (parser == &ug_html_parser_custom) {
			// call custom start_element
			((UgHtmlParserStartElementFunc)entry->param1) (uhtml,
					element_name, attribute_names,
					attribute_values, dest, (void*) entry);
		}
		else
			ug_html_push (uhtml, parser, dest, (void*) entry);
		return;
	}

	ug_html_push (uhtml, &ug_html_parser_unknown, NULL, NULL);
}

const UgHtmlParser ug_html_parser_entry =
{
	(void*) ug_html_start_element_entry,
	(void*) ug_html_pop,
	(void*) NULL,
};

