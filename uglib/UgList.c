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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_GLIB
#include <glib.h>    // g_malloc/g_free
#endif // HAVE_GLIB

#include <stdlib.h>
#include <string.h>
#include <UgString.h>
#include <UgList.h>
#include <UgEntry.h>

// ----------------------------------------------------------------------------
// UgLink

UgLink* ug_link_new (void)
{
#ifdef HAVE_GLIB
	return g_malloc0 (sizeof (UgLink));
#else
	return ug_malloc0 (sizeof (UgLink));
#endif
}

void  ug_link_free (UgLink* link)
{
#ifdef HAVE_GLIB
	g_free (link);
#else
	ug_free (link);
#endif
}

// ----------------------------------------------------------------------------
// UgList: for UG_ENTRY_LIST

void  ug_list_init  (UgList* list)
{
	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
}

void  ug_list_clear (UgList* list, int free_links)
{
	UgLink*  link;
	UgLink*  next;

	if (free_links) {
		for (link = list->head;  link;  link = next) {
			next = link->next;
//			ug_link_free (link);
#ifdef HAVE_GLIB
			g_free (link);
#else
			ug_free (link);
#endif // HAVE_GLIB
		}
	}

	list->size = 0;
	list->head = NULL;
	list->tail = NULL;
}

void  ug_list_foreach (UgList* list, UgForeachFunc func, void* data)
{
	UgLink*  link;
	UgLink*  next;

	for (link = list->head;  link;  link = next) {
		next = link->next;
		func (link->data, data);
	}
}

void  ug_list_foreach_link (UgList* list, UgForeachFunc func, void* data)
{
	UgLink*  link;
	UgLink*  next;

	for (link = list->head;  link;  link = next) {
		next = link->next;
		func (link, data);
	}
}

void  ug_list_prepend (UgList* list, UgLink* link)
{
	link->prev = NULL;
	link->next = list->head;
	if (list->head)
		list->head->prev = link;
	else // if (list->tail == NULL)
		list->tail = link;
	list->head = link;
	list->size++;
}

void  ug_list_append  (UgList* list, UgLink* link)
{
	if (list->tail)
		list->tail->next = link;
	else // if (list->head == NULL)
		list->head = link;
	link->next = NULL;
	link->prev = list->tail;
	list->tail = link;
	list->size++;
}

void  ug_list_insert  (UgList* list, UgLink* sibling, UgLink* link)
{
	if (sibling == NULL) {
		ug_list_append (list, link);
		return;
	}
	if (sibling == list->head) {
		ug_list_prepend (list, link);
		return;
	}

	sibling->prev->next = link;
	link->next = sibling;
	link->prev = sibling->prev;
	sibling->prev = link;
	list->size++;
}

void  ug_list_remove  (UgList* list, UgLink* link)
{
	if (list->tail == link)
		list->tail = link->prev;
	if (list->head == link)
		list->head = link->next;

	if (link->prev)
		link->prev->next = link->next;
	if (link->next)
		link->next->prev = link->prev;
	link->next = NULL;
	link->prev = NULL;
	list->size--;
}

int  ug_list_position (UgList* list, UgLink* link)
{
	UgLink*  temp;
	int      pos;

	for (pos = 0, temp = list->head;  temp;  temp = temp->next, pos++) {
		if (temp == link)
			return pos;
	}
	return -1;
}

// ----------------------------------------------------------------------------
// UgJsonParseFunc for JSON array elements

UgJsonError ug_json_parse_list_bool (UgJson* json,
                                     const char* name, const char* value,
                                     void* list, void* none)
{
	UgLink*  link;

	if (json->type != UG_JSON_TRUE && json->type != UG_JSON_FALSE) {
//		if (json->type >= UG_JSON_OBJECT)
//			ug_json_push (json, ug_json_parse_unknown, NULL, NULL);
		return UG_JSON_ERROR_TYPE_NOT_MATCH;
	}

	link = ug_link_new ();
	if (json->type == UG_JSON_TRUE)
		link->data = (void*)(intptr_t)1;
	else
		link->data = (void*)(intptr_t)0;

	ug_list_append (list, link);
	return UG_JSON_ERROR_NONE;
}

UgJsonError ug_json_parse_list_int (UgJson* json,
                                    const char* name, const char* value,
                                    void* list, void* none)
{
	UgLink*  link;

	if (json->type != UG_JSON_NUMBER) {
//		if (json->type >= UG_JSON_OBJECT)
//			ug_json_push (json, ug_json_parse_unknown, NULL, NULL);
		return UG_JSON_ERROR_TYPE_NOT_MATCH;
	}

	link = ug_link_new ();
	link->data = (void*)(intptr_t) strtol (value, NULL, 10);
	ug_list_append (list, link);
	return UG_JSON_ERROR_NONE;
}

UgJsonError ug_json_parse_list_uint (UgJson* json,
                                     const char* name, const char* value,
                                     void* list, void* none)
{
	UgLink*  link;

	if (json->type != UG_JSON_NUMBER) {
//		if (json->type >= UG_JSON_OBJECT)
//			ug_json_push (json, ug_json_parse_unknown, NULL, NULL);
		return UG_JSON_ERROR_TYPE_NOT_MATCH;
	}

	link = ug_link_new ();
	link->data = (void*)(uintptr_t) strtoul (value, NULL, 10);
	ug_list_append (list, link);
	return UG_JSON_ERROR_NONE;
}

UgJsonError ug_json_parse_list_string (UgJson* json,
                                       const char* name, const char* value,
                                       void* list, void* none)
{
	UgLink*  link;
	char*    string;

	if (json->type == UG_JSON_STRING)
		string = ug_strdup (value);
	else if (json->type == UG_JSON_NULL)
		string = NULL;
	else {
//		if (json->type >= UG_JSON_OBJECT)
//			ug_json_push (json, ug_json_parse_unknown, NULL, NULL);
		return UG_JSON_ERROR_TYPE_NOT_MATCH;
	}

	link = ug_link_new ();
	link->data = string;
	ug_list_append (list, link);
	return UG_JSON_ERROR_NONE;
}

// ----------------------------------------------------------------------------
// write JSON array elements

void	ug_json_write_list_bool (UgJson* json, UgList* list)
{
	UgLink*  link;

	for (link = list->head;  link;  link = link->next)
		ug_json_write_bool (json, (intptr_t)link->data);
}

void	ug_json_write_list_int (UgJson* json, UgList* list)
{
	UgLink*  link;

	for (link = list->head;  link;  link = link->next)
		ug_json_write_int (json, (intptr_t)link->data);
}

void	ug_json_write_list_uint (UgJson* json, UgList* list)
{
	UgLink*  link;

	for (link = list->head;  link;  link = link->next)
		ug_json_write_uint (json, (uintptr_t)link->data);
}

void	ug_json_write_list_string (UgJson* json, UgList* list)
{
	UgLink*  link;

	for (link = list->head;  link;  link = link->next) {
		if (link->data == NULL)
			ug_json_write_null (json);
		else
			ug_json_write_string (json, link->data);
	}
}

