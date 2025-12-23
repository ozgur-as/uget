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

#include <strings.h>
#include <UgArray.h>
#include <UgString.h>
#include <UgHtmlFilter.h>

static UgHtmlParser	default_parser;
static UgHtmlParser	script_parser;
static UgHtmlParser	head_parser;

UgHtmlFilter*  ug_html_filter_new (void)
{
	UgHtmlFilter* filter;

	filter = ug_malloc (sizeof (UgHtmlFilter));
	ug_html_init ((UgHtml*) filter);
	filter->base_href = NULL;
	filter->charset = NULL;

	ug_list_init (&filter->tags);
	ug_html_push ((UgHtml*)filter, &default_parser, NULL, NULL);
	return (UgHtmlFilter*) filter;
}

void  ug_html_filter_free (UgHtmlFilter* filter)
{
	ug_free (filter->base_href);
	ug_free (filter->charset);
	ug_list_foreach (&filter->tags, (UgForeachFunc) ug_html_filter_tag_unref, NULL);
	ug_list_clear (&filter->tags, FALSE);
	ug_free (filter);
}

void  ug_html_filter_add_tag (UgHtmlFilter* filter, UgHtmlFilterTag* tag)
{
	ug_list_prepend (&filter->tags, (UgLink*) tag);
	ug_html_filter_tag_ref (tag);
	tag->filter = filter;
}

int  ug_html_filter_parse_file (UgHtmlFilter* filter, const char* file_utf8)
{
	UgHtml*  uhtml;

	uhtml = (UgHtml*) filter;
	ug_html_push (uhtml, &default_parser, NULL, NULL);
	return ug_html_parse_file (uhtml, file_utf8);
}


// ----------------------------------------------------------------------------
// UgHtmlFilterTag

UgHtmlFilterTag*  ug_html_filter_tag_new (char* element_name, char* attr_name)
{
	UgHtmlFilterTag*  tag;

	tag = ug_malloc0 (sizeof (UgHtmlFilterTag));
	tag->self = tag;
	tag->tag_name  = ug_strdup (element_name);
	tag->attr_name = ug_strdup (attr_name);
	ug_list_init (&tag->attr_values);
	tag->ref_count = 1;
	return tag;
}

void  ug_html_filter_tag_ref   (UgHtmlFilterTag* tag)
{
	tag->ref_count++;
}

void  ug_html_filter_tag_unref (UgHtmlFilterTag* tag)
{
	tag->ref_count--;
	if (tag->ref_count == 0) {
		ug_free (tag->tag_name);
		ug_free (tag->attr_name);
		ug_list_foreach (&tag->attr_values, (UgForeachFunc) ug_free, NULL);
		ug_list_clear (&tag->attr_values, TRUE);
		ug_free (tag);
	}
}


// ----------------------------------------------------------------------------
// default parser

static void	cb_start_element (UgHtml*        uhtml,
                              const char*    element_name,
                              const char**   attribute_names,
                              const char**   attribute_values,
                              void*          dest)
{
	UgHtmlFilter*     filter = (UgHtmlFilter*) uhtml;
	UgHtmlFilterTag*  tag;
	UgLink*   link;
	UgLink*   strlink;

	if (strcasecmp (element_name, "script") == 0) {
		ug_html_push (uhtml, &script_parser, NULL, NULL);
		return;
	}

	if (strcasecmp (element_name, "head") == 0) {
		ug_html_push (uhtml, &head_parser, NULL, NULL);
		return;
	}

	// filter tag
	for (link = filter->tags.head;  link;  link = link->next) {
		tag = link->data;
		if (strcasecmp (element_name, tag->tag_name) != 0)
			continue;
		for (; *attribute_names; attribute_names++, attribute_values++) {
			if (strcasecmp (*attribute_names, tag->attr_name) != 0)
				continue;
			if (tag->callback)
				tag->callback (tag, *attribute_values, tag->callback_data);
			else {
				strlink = ug_link_new ();
				strlink->data = ug_strdup (*attribute_values);
				ug_list_prepend (&tag->attr_values, strlink);
			}
		}
	}
}

static UgHtmlParser	default_parser =
{
	(void*) cb_start_element,
	(void*) NULL,
	(void*) NULL,
};

// ----------------------------------------------------------------------------
// parser - <script></srcipt>

static void	cb_end_script (UgHtml*        uhtml,
                           const char*    element_name,
                           void*          user_data)
{
	if (strcasecmp (element_name, "script") == 0)
		ug_html_pop (uhtml);
}

static UgHtmlParser	script_parser =
{
	(void*) NULL,
	(void*) cb_end_script,
	(void*) NULL,
};

// ----------------------------------------------------------------------------
// parser - <head></head>

static void	cb_start_head (UgHtml*        uhtml,
                           const char*    element_name,
                           const char**   attribute_names,
                           const char**   attribute_values,
                           void*          user_data)
{
	UgHtmlFilter*    filter = (UgHtmlFilter*) uhtml;
	UgHtmlFilterTag* tag;
	UgLink* link;
	UgLink* strlink;
	char*   string;

	// <script>
	if (strcasecmp (element_name, "script") == 0) {
		ug_html_push (uhtml, &script_parser, NULL, NULL);
		return;
	}

	// get charset
	if (filter->charset == NULL && strcasecmp (element_name, "meta") == 0) {
		for (; *attribute_names; attribute_names++, attribute_values++) {
			if (strcasecmp (*attribute_names, "content") != 0)
				continue;
			string = strstr (*attribute_values, "charset=");
			if (string == NULL)
				continue;
			ug_free (filter->charset);
			filter->charset = ug_strdup (string + 8);
			break;
		}
		return;
	}

	// base href
	if (strcasecmp (element_name, "base") == 0) {
		for (; *attribute_names; attribute_names++, attribute_values++) {
			if (strcasecmp (*attribute_names, "href") != 0)
				continue;
			ug_free (filter->base_href);
			filter->base_href = ug_strdup (*attribute_values);
			break;
		}
		return;
	}

	// filter
	for (link = filter->tags.head;  link;  link = link->next) {
		tag = link->data;
		if (strcasecmp (element_name, tag->tag_name) != 0)
			continue;
		for (; *attribute_names; attribute_names++, attribute_values++) {
			if (strcasecmp (*attribute_names, tag->attr_name) != 0)
				continue;
			if (tag->callback)
				tag->callback (tag, *attribute_values, tag->callback_data);
			else {
				strlink = ug_link_new ();
				strlink->data = ug_strdup (*attribute_values);
				ug_list_prepend (&tag->attr_values, strlink);
			}
		}
	}
}

static void cb_end_head (UgHtml*        uhtml,
                         const char*    element_name,
                         void*          user_data)
{
	if (strcasecmp (element_name, "head") == 0)
		ug_html_pop (uhtml);
}

static UgHtmlParser	head_parser =
{
	(void*) cb_start_head,
	(void*) cb_end_head,
	(void*) NULL,
};

