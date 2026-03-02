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

#ifndef UGTK_SELECTOR_H
#define UGTK_SELECTOR_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct  UgtkSelector        UgtkSelector;
typedef struct  UgtkSelectorPage    UgtkSelectorPage;
typedef	void  (*UgtkSelectorNotify) (gpointer user_data, gboolean completed);

// ----------------------------------------------------------------------------
// UgtkSelector
//
struct UgtkSelector
{
	GtkWidget*    self;     // GtkBox (vertical)
	GtkWindow*    parent;   // parent window of UgtkSelector.self

	GtkNotebook*  notebook;
	// <base href>
	GtkWidget*    href_label;
	GtkEntry*     href_entry;       // entry for hypertext reference
	GtkWidget*    href_separator;
	// select button
	GtkWidget*    select_all;
	GtkWidget*    select_none;
	GtkWidget*    select_filter;    // select by filter

	// UgtkSelectorPage is placed in array
	GArray*       pages;

	// UgtkSelectorFilter use UgtkSelectorFilterData in UgtkSelectorPage
	struct UgtkSelectorFilter
	{
		GtkWindow*    dialog;
		GtkListBox*   host_list;
		GtkListBox*   ext_list;
	} filter;

	// callback
	struct
	{
		UgtkSelectorNotify  func;
		gpointer            data;
	} notify;
};

void  ugtk_selector_init (UgtkSelector* selector, GtkWindow* parent);
void  ugtk_selector_finalize (UgtkSelector* selector);

void  ugtk_selector_hide_href (UgtkSelector* selector);

// (gchar*) list->data.
// To free the returned value, use:
//	g_list_free_full (list, (GDestroyNotify) g_free);
GList* ugtk_selector_get_marked_uris (UgtkSelector* selector);

// count marked item and notify
gint   ugtk_selector_count_marked (UgtkSelector* selector);
gint   ugtk_selector_n_items (UgtkSelector* selector);

UgtkSelectorPage*   ugtk_selector_add_page (UgtkSelector* selector, const gchar* title);
UgtkSelectorPage*   ugtk_selector_get_page (UgtkSelector* selector, gint nth_page);

// ----------------------------------------------------------------------------
// UgtkSelectorPage
//
struct UgtkSelectorPage
{
	GtkWidget*     self;    // GtkScrolledWindow

	GtkListBox*    list_box;
	GPtrArray*     items;     // UgtkSelectorItem*

	// total marked count
	gint    n_marked;

	// used by UgtkSelectorFilter
	struct UgtkSelectorFilterData
	{
		GHashTable*    hash;
		GPtrArray*     host;    // UgtkSelectorItem*
		GPtrArray*     ext;     // UgtkSelectorItem*
	} filter;
};

void  ugtk_selector_page_init (UgtkSelectorPage* page);
void  ugtk_selector_page_finalize (UgtkSelectorPage* page);

// return numbers of uri added
int   ugtk_selector_page_add_uris (UgtkSelectorPage* page, GList* uris);
void  ugtk_selector_page_make_filter (UgtkSelectorPage* page);
void  ugtk_selector_page_mark_by_filter_all (UgtkSelectorPage* page);


#ifdef __cplusplus
}
#endif

#endif  // End of UGTK_SELECTOR_H
