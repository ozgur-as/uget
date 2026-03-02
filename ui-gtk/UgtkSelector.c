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

#include <UgetNode.h>
#include <UgetData.h>
#include <UgtkSelector.h>
#include <UgUri.h>
#include <UgtkApp.h>        // UGTK_APP_NAME

#include <glib/gi18n.h>


// ----------------------------------------------------------------------------
// UgtkSelectorItem
//
typedef struct	UgtkSelectorItem    UgtkSelectorItem;

struct UgtkSelectorItem
{
	gboolean  mark;
	gchar*    uri;
	void*     data;
};

// GPtrArray helpers for UgtkSelectorItem
static GList* ugtk_selector_items_get_marked   (GPtrArray* items, GList* list);
static void   ugtk_selector_items_set_mark_all (GPtrArray* items, gboolean mark);
static void   ugtk_selector_items_clear        (GPtrArray* items);

// ----------------------------------------------------------------------------
// UgtkSelector
//
static void ugtk_selector_init_ui (UgtkSelector* selector);
static void ugtk_selector_filter_init (struct UgtkSelectorFilter* filter, UgtkSelector* selector);
static void ugtk_selector_filter_show (struct UgtkSelectorFilter* filter, UgtkSelectorPage* page);
// forward declarations
void ugtk_selector_page_rebuild_list (UgtkSelectorPage* page, UgtkSelector* selector);
// signal handlers
static void on_selector_mark_all    (GtkWidget* button, UgtkSelector* selector);
static void on_selector_mark_none   (GtkWidget* button, UgtkSelector* selector);
static void on_selector_mark_filter (GtkWidget* button, UgtkSelector* selector);
static void on_filter_ok (GtkWidget* button, UgtkSelector* selector);
static void on_filter_cancel (GtkWidget* button, UgtkSelector* selector);
static gboolean on_filter_close_request (GtkWindow* window, UgtkSelector* selector);
static void on_filter_button_all (GtkWidget* widget, gpointer user_data);
static void on_filter_button_none (GtkWidget* widget, gpointer user_data);

void  ugtk_selector_init (UgtkSelector* selector, GtkWindow* parent)
{
	selector->parent = parent;
	ugtk_selector_init_ui (selector);
	// UgtkSelectorPage initialize
	selector->pages = g_array_new (FALSE, FALSE, sizeof (UgtkSelectorPage));
	// UgtkSelectorFilter initialize
	ugtk_selector_filter_init (&selector->filter, selector);

	g_signal_connect (selector->select_all, "clicked",
			G_CALLBACK (on_selector_mark_all), selector);
	g_signal_connect (selector->select_none, "clicked",
			G_CALLBACK (on_selector_mark_none), selector);
	g_signal_connect (selector->select_filter, "clicked",
			G_CALLBACK (on_selector_mark_filter), selector);
}

void  ugtk_selector_finalize (UgtkSelector* selector)
{
	UgtkSelectorPage*  page;
	GArray*  array;
	guint    index;

	// UgtkSelectorPage finalize
	array = selector->pages;
	for (index=0; index < array->len; index++) {
		page = &g_array_index (array, UgtkSelectorPage, index);
		ugtk_selector_page_finalize (page);
	}
	g_array_free (selector->pages, TRUE);

	// UgtkSelectorFilter finalize
	gtk_window_destroy (GTK_WINDOW (selector->filter.dialog));
}

void  ugtk_selector_hide_href (UgtkSelector* selector)
{
	gtk_widget_set_visible ((GtkWidget*) selector->href_label, FALSE);
	gtk_widget_set_visible ((GtkWidget*) selector->href_entry, FALSE);
	gtk_widget_set_visible ((GtkWidget*) selector->href_separator, FALSE);
}

static GList*  ugtk_selector_get_marked (UgtkSelector* selector)
{
	UgtkSelectorPage*  page;
	GList*  list;
	guint   index;

	list = NULL;
	for (index = 0;  index < selector->pages->len;  index++) {
		page = &g_array_index (selector->pages, UgtkSelectorPage, index);
		list = ugtk_selector_items_get_marked (page->items, list);
	}
	return g_list_reverse (list);
}

GList*  ugtk_selector_get_marked_uris (UgtkSelector* selector)
{
	UgtkSelectorItem*  item;
	GString*     gstr;
	GList*       list;
	GList*       link;
	const gchar* base_href;

	list = ugtk_selector_get_marked (selector);
	base_href = gtk_editable_get_text (GTK_EDITABLE (selector->href_entry));
	if (base_href[0] == 0)
		base_href = NULL;
	for (link = list;  link;  link = link->next) {
		item = link->data;
		// URI list
		if (ug_uri_init (NULL, item->uri) == 0 && base_href) {
			gstr = g_string_new (base_href);
			if (gstr->str[gstr->len -1] == '/') {
				if (item->uri[0] == '/')
					g_string_truncate (gstr, gstr->len -1);
			}
			else if (item->uri[0] != '/')
				g_string_append_c (gstr, '/');
			g_string_append (gstr, item->uri);
		}
		else
			gstr = g_string_new (item->uri);
		link->data = g_string_free (gstr, FALSE);
	}
	return list;
}

gint  ugtk_selector_count_marked (UgtkSelector* selector)
{
	gint   count;
	guint  index;

	count = 0;
	for (index = 0;  index < selector->pages->len;  index++) {
		count += g_array_index (selector->pages, UgtkSelectorPage, index).n_marked;
	}
	if (selector->notify.func)
		selector->notify.func (selector->notify.data, (count) ? TRUE: FALSE);
	return count;
}

gint   ugtk_selector_n_items (UgtkSelector* selector)
{
	gint   count;
	guint  index;
	UgtkSelectorPage* page;

	count = 0;
	for (index = 0;  index < selector->pages->len;  index++) {
		page = &g_array_index (selector->pages, UgtkSelectorPage, index);
		count += page->items->len;
	}
	return count;
}

UgtkSelectorPage*  ugtk_selector_add_page (UgtkSelector* selector, const gchar* title)
{
	UgtkSelectorPage* page;
	GArray*           array;

	array = selector->pages;
	g_array_set_size (array, array->len + 1);
	page = &g_array_index (array, UgtkSelectorPage, array->len - 1);
	ugtk_selector_page_init (page);

	gtk_notebook_append_page (selector->notebook, page->self, gtk_label_new (title));
	return page;
}

UgtkSelectorPage*  ugtk_selector_get_page (UgtkSelector* selector, gint nth_page)
{
	UgtkSelectorPage* page;

	if (nth_page < 0)
		nth_page = gtk_notebook_get_current_page (selector->notebook);
	if (nth_page == -1 || nth_page >= (gint)selector->pages->len)
		return NULL;
	page = &g_array_index (selector->pages, UgtkSelectorPage, nth_page);
	return page;
}


// ----------------------------------------------------------------------------
// UgtkSelectorFilter use UgtkSelectorFilterData in UgtkSelectorPage
//

static void ugtk_selector_filter_rebuild_list (GtkListBox* list_box, GPtrArray* items)
{
	GtkWidget* child;
	guint i;

	// Clear existing rows
	while ((child = gtk_widget_get_first_child (GTK_WIDGET (list_box))))
		gtk_list_box_remove (list_box, child);

	// Add rows
	for (i = 0; i < items->len; i++) {
		UgtkSelectorItem* item = g_ptr_array_index (items, i);
		GtkWidget* box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
		GtkWidget* check = gtk_check_button_new ();
		GtkWidget* label = gtk_label_new (item->uri);

		gtk_check_button_set_active (GTK_CHECK_BUTTON (check), item->mark);
		gtk_label_set_xalign (GTK_LABEL (label), 0.0);
		gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
		gtk_widget_set_hexpand (label, TRUE);
		gtk_box_append (GTK_BOX (box), check);
		gtk_box_append (GTK_BOX (box), label);
		gtk_list_box_append (list_box, box);

		// Store item pointer on the check button
		g_object_set_data (G_OBJECT (check), "selector-item", item);
	}
}

static void on_filter_check_toggled (GtkCheckButton* check, gpointer user_data)
{
	UgtkSelectorItem* item = g_object_get_data (G_OBJECT (check), "selector-item");
	if (item)
		item->mark = gtk_check_button_get_active (check);
}

static void ugtk_selector_filter_connect_toggles (GtkListBox* list_box)
{
	GtkWidget* row;
	int i;

	for (i = 0; ; i++) {
		row = GTK_WIDGET (gtk_list_box_get_row_at_index (list_box, i));
		if (!row) break;
		GtkWidget* box = gtk_list_box_row_get_child (GTK_LIST_BOX_ROW (row));
		GtkWidget* check = gtk_widget_get_first_child (box);
		if (check)
			g_signal_connect (check, "toggled", G_CALLBACK (on_filter_check_toggled), NULL);
	}
}

static void ugtk_selector_filter_init (struct UgtkSelectorFilter* filter, UgtkSelector* selector)
{
	GtkWindow*  window;
	GtkWidget*  widget;
	GtkBox*     vbox;
	GtkBox*     hbox;
	GtkBox*     button_box;
	GtkWidget*  cancel_button;
	GtkWidget*  ok_button;
	gchar*      title;

	title  = g_strconcat (UGTK_APP_NAME " - ", _("Mark by filter"), NULL);
	window = (GtkWindow*) gtk_window_new ();
	gtk_window_set_title (window, title);
	g_free (title);
	gtk_window_set_modal (window, FALSE);
	gtk_window_set_destroy_with_parent (window, TRUE);
	gtk_window_set_transient_for (window, selector->parent);
	gtk_window_set_default_size (window, 480, 330);
	filter->dialog = window;

	// main layout
	vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	gtk_window_set_child (window, (GtkWidget*) vbox);
	gtk_box_append (vbox,
			gtk_label_new (_("Mark URLs by host AND filename extension.")));
	gtk_box_append (vbox,
			gtk_label_new (_("This will reset all marks of URLs.")));

	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_widget_set_vexpand ((GtkWidget*) hbox, TRUE);
	gtk_box_append (vbox, (GtkWidget*) hbox);

	// filter views - host
	filter->host_list = GTK_LIST_BOX (gtk_list_box_new ());
	{
		GtkBox* host_vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
		gtk_box_append (host_vbox, gtk_label_new (_("Host")));
		GtkWidget* host_scroll = gtk_scrolled_window_new ();
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (host_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_widget_set_vexpand (host_scroll, TRUE);
		gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (host_scroll), GTK_WIDGET (filter->host_list));
		gtk_box_append (host_vbox, host_scroll);
		GtkBox* host_btn_box = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
		widget = gtk_button_new_with_label (_("All"));
		g_object_set_data (G_OBJECT (widget), "filter-list", filter->host_list);
		g_signal_connect (widget, "clicked", G_CALLBACK (on_filter_button_all), NULL);
		gtk_box_append (host_btn_box, widget);
		widget = gtk_button_new_with_label (_("None"));
		g_object_set_data (G_OBJECT (widget), "filter-list", filter->host_list);
		g_signal_connect (widget, "clicked", G_CALLBACK (on_filter_button_none), NULL);
		gtk_box_append (host_btn_box, widget);
		gtk_box_append (host_vbox, (GtkWidget*) host_btn_box);
		gtk_widget_set_hexpand ((GtkWidget*) host_vbox, TRUE);
		gtk_box_append (hbox, (GtkWidget*) host_vbox);
	}
	// filter views - ext
	filter->ext_list = GTK_LIST_BOX (gtk_list_box_new ());
	{
		GtkBox* ext_vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
		gtk_box_append (ext_vbox, gtk_label_new (_("File Ext.")));
		GtkWidget* ext_scroll = gtk_scrolled_window_new ();
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ext_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_widget_set_vexpand (ext_scroll, TRUE);
		gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (ext_scroll), GTK_WIDGET (filter->ext_list));
		gtk_box_append (ext_vbox, ext_scroll);
		GtkBox* ext_btn_box = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
		widget = gtk_button_new_with_label (_("All"));
		g_object_set_data (G_OBJECT (widget), "filter-list", filter->ext_list);
		g_signal_connect (widget, "clicked", G_CALLBACK (on_filter_button_all), NULL);
		gtk_box_append (ext_btn_box, widget);
		widget = gtk_button_new_with_label (_("None"));
		g_object_set_data (G_OBJECT (widget), "filter-list", filter->ext_list);
		g_signal_connect (widget, "clicked", G_CALLBACK (on_filter_button_none), NULL);
		gtk_box_append (ext_btn_box, widget);
		gtk_box_append (ext_vbox, (GtkWidget*) ext_btn_box);
		gtk_widget_set_hexpand ((GtkWidget*) ext_vbox, TRUE);
		gtk_box_append (hbox, (GtkWidget*) ext_vbox);
	}

	// button box
	button_box = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_set_halign ((GtkWidget*) button_box, GTK_ALIGN_END);
	gtk_widget_set_margin_top ((GtkWidget*) button_box, 6);
	gtk_widget_set_margin_bottom ((GtkWidget*) button_box, 6);
	gtk_widget_set_margin_end ((GtkWidget*) button_box, 6);
	gtk_box_append (vbox, (GtkWidget*) button_box);
	cancel_button = gtk_button_new_with_mnemonic (_("_Cancel"));
	gtk_box_append (button_box, cancel_button);
	ok_button = gtk_button_new_with_mnemonic (_("_OK"));
	gtk_widget_add_css_class (ok_button, "suggested-action");
	gtk_box_append (button_box, ok_button);

	g_signal_connect (ok_button, "clicked",
			G_CALLBACK (on_filter_ok), selector);
	g_signal_connect (cancel_button, "clicked",
			G_CALLBACK (on_filter_cancel), selector);
	g_signal_connect (window, "close-request",
			G_CALLBACK (on_filter_close_request), selector);
}

static void ugtk_selector_filter_show (struct UgtkSelectorFilter* filter, UgtkSelectorPage* page)
{
	GtkWindow* parent;

	ugtk_selector_filter_rebuild_list (filter->host_list, page->filter.host);
	ugtk_selector_filter_connect_toggles (filter->host_list);
	ugtk_selector_filter_rebuild_list (filter->ext_list, page->filter.ext);
	ugtk_selector_filter_connect_toggles (filter->ext_list);

	parent = gtk_window_get_transient_for ((GtkWindow*) filter->dialog);
	if (parent)
		gtk_widget_set_sensitive ((GtkWidget*) parent, FALSE);
	gtk_widget_set_visible ((GtkWidget*) filter->dialog, TRUE);
}

//	signal handler ------------------------------

static void on_item_check_toggled (GtkCheckButton* check, UgtkSelector* selector)
{
	UgtkSelectorItem* item = g_object_get_data (G_OBJECT (check), "selector-item");
	UgtkSelectorPage* page = ugtk_selector_get_page (selector, -1);
	if (!item || !page) return;

	gboolean new_mark = gtk_check_button_get_active (check);
	if (new_mark != item->mark) {
		item->mark = new_mark;
		if (item->mark)
			page->n_marked++;
		else
			page->n_marked--;
		ugtk_selector_count_marked (selector);
	}
}

static void on_selector_mark_all (GtkWidget* button, UgtkSelector* selector)
{
	UgtkSelectorPage*  page;

	page = ugtk_selector_get_page (selector, -1);
	if (page == NULL)
		return;
	ugtk_selector_items_set_mark_all (page->items, TRUE);
	page->n_marked = page->items->len;
	ugtk_selector_page_rebuild_list (page, selector);
	ugtk_selector_count_marked (selector);
}

static void on_selector_mark_none (GtkWidget* button, UgtkSelector* selector)
{
	UgtkSelectorPage*  page;

	page = ugtk_selector_get_page (selector, -1);
	if (page == NULL)
		return;
	ugtk_selector_items_set_mark_all (page->items, FALSE);
	page->n_marked = 0;
	ugtk_selector_page_rebuild_list (page, selector);
	ugtk_selector_count_marked (selector);
}

static void on_filter_button_all (GtkWidget* widget, gpointer user_data)
{
	GtkListBox* list_box = g_object_get_data (G_OBJECT (widget), "filter-list");
	if (!list_box) return;
	// Set all checkboxes to active
	for (int i = 0; ; i++) {
		GtkWidget* row = GTK_WIDGET (gtk_list_box_get_row_at_index (list_box, i));
		if (!row) break;
		GtkWidget* box = gtk_list_box_row_get_child (GTK_LIST_BOX_ROW (row));
		GtkWidget* check = gtk_widget_get_first_child (box);
		if (check) {
			UgtkSelectorItem* item = g_object_get_data (G_OBJECT (check), "selector-item");
			if (item) item->mark = TRUE;
			gtk_check_button_set_active (GTK_CHECK_BUTTON (check), TRUE);
		}
	}
}

static void on_filter_button_none (GtkWidget* widget, gpointer user_data)
{
	GtkListBox* list_box = g_object_get_data (G_OBJECT (widget), "filter-list");
	if (!list_box) return;
	for (int i = 0; ; i++) {
		GtkWidget* row = GTK_WIDGET (gtk_list_box_get_row_at_index (list_box, i));
		if (!row) break;
		GtkWidget* box = gtk_list_box_row_get_child (GTK_LIST_BOX_ROW (row));
		GtkWidget* check = gtk_widget_get_first_child (box);
		if (check) {
			UgtkSelectorItem* item = g_object_get_data (G_OBJECT (check), "selector-item");
			if (item) item->mark = FALSE;
			gtk_check_button_set_active (GTK_CHECK_BUTTON (check), FALSE);
		}
	}
}

static void on_selector_mark_filter (GtkWidget* button, UgtkSelector* selector)
{
	UgtkSelectorPage*  page;

	page = ugtk_selector_get_page (selector, -1);
	if (page == NULL)
		return;
	ugtk_selector_page_make_filter (page);
	ugtk_selector_filter_show (&selector->filter, page);
}

static void on_filter_ok (GtkWidget* button, UgtkSelector* selector)
{
	UgtkSelectorPage*  page;
	GtkWindow*  parent;

	page = ugtk_selector_get_page (selector, -1);
	if (page)
		ugtk_selector_page_mark_by_filter_all (page);
	parent = gtk_window_get_transient_for (selector->filter.dialog);
	if (parent)
		gtk_widget_set_sensitive ((GtkWidget*) parent, TRUE);
	gtk_widget_set_visible ((GtkWidget*) selector->filter.dialog, FALSE);
	// Rebuild main page list to reflect changes
	if (page)
		ugtk_selector_page_rebuild_list (page, selector);
	ugtk_selector_count_marked (selector);
}

static void on_filter_cancel (GtkWidget* button, UgtkSelector* selector)
{
	GtkWindow*  parent;

	parent = gtk_window_get_transient_for (selector->filter.dialog);
	if (parent)
		gtk_widget_set_sensitive ((GtkWidget*) parent, TRUE);
	gtk_widget_set_visible ((GtkWidget*) selector->filter.dialog, FALSE);
	ugtk_selector_count_marked (selector);
}

static gboolean on_filter_close_request (GtkWindow* window, UgtkSelector* selector)
{
	on_filter_cancel (NULL, selector);
	return TRUE;
}


// ----------------------------------------------------------------------------
// UgtkSelectorPage
//

void ugtk_selector_page_rebuild_list (UgtkSelectorPage* page, UgtkSelector* selector)
{
	GtkWidget* child;
	guint i;

	while ((child = gtk_widget_get_first_child (GTK_WIDGET (page->list_box))))
		gtk_list_box_remove (page->list_box, child);

	for (i = 0; i < page->items->len; i++) {
		UgtkSelectorItem* item = g_ptr_array_index (page->items, i);
		GtkWidget* box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
		GtkWidget* check = gtk_check_button_new ();
		GtkWidget* label = gtk_label_new (item->uri);

		gtk_check_button_set_active (GTK_CHECK_BUTTON (check), item->mark);
		gtk_label_set_xalign (GTK_LABEL (label), 0.0);
		gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
		gtk_widget_set_hexpand (label, TRUE);
		gtk_box_append (GTK_BOX (box), check);
		gtk_box_append (GTK_BOX (box), label);
		gtk_list_box_append (page->list_box, box);

		g_object_set_data (G_OBJECT (check), "selector-item", item);
		if (selector)
			g_signal_connect (check, "toggled", G_CALLBACK (on_item_check_toggled), selector);
	}
}

void  ugtk_selector_page_init (UgtkSelectorPage* page)
{
	GtkScrolledWindow*  scrolled;

	page->items = g_ptr_array_new ();
	page->list_box = GTK_LIST_BOX (gtk_list_box_new ());
	gtk_list_box_set_selection_mode (page->list_box, GTK_SELECTION_NONE);

	page->self = gtk_scrolled_window_new ();
	scrolled = GTK_SCROLLED_WINDOW (page->self);
	gtk_scrolled_window_set_policy (scrolled,
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled), GTK_WIDGET (page->list_box));
	gtk_widget_set_visible(page->self, TRUE);

	page->n_marked = 0;

	// UgtkSelectorFilterData initialize
	page->filter.hash = g_hash_table_new_full (g_str_hash, g_str_equal,
			NULL, (GDestroyNotify) g_list_free);
	page->filter.host = g_ptr_array_new ();
	page->filter.ext  = g_ptr_array_new ();
}

void  ugtk_selector_page_finalize (UgtkSelectorPage* page)
{
	ugtk_selector_items_clear (page->items);
	g_ptr_array_free (page->items, TRUE);

	// UgtkSelectorFilterData finalize
	g_hash_table_destroy (page->filter.hash);
	ugtk_selector_items_clear (page->filter.host);
	g_ptr_array_free (page->filter.host, TRUE);
	ugtk_selector_items_clear (page->filter.ext);
	g_ptr_array_free (page->filter.ext, TRUE);
}

int  ugtk_selector_page_add_uris (UgtkSelectorPage* page, GList* uris)
{
	UgtkSelectorItem* item;
	int  counts;

	for (counts = 0;  uris;  uris = uris->next) {
		if (uris->data == NULL)
			continue;
		counts++;
		item = g_slice_alloc (sizeof (UgtkSelectorItem));
		item->mark = TRUE;
		item->uri  = uris->data;
		item->data = NULL;
		uris->data = NULL;  // item->uri
		g_ptr_array_add (page->items, item);
		page->n_marked++;
	}
	return counts;
}

static void ugtk_selector_page_add_filter (UgtkSelectorPage* page, GPtrArray* filter_items, gchar* key, UgtkSelectorItem* value)
{
	UgtkSelectorItem* filter_item;
	GList*       filter_list;
	gchar*       orig_key;

	if (g_hash_table_lookup_extended (page->filter.hash, key,
			(gpointer*) &orig_key, (gpointer*) &filter_list) == FALSE)
	{
		filter_item = g_slice_alloc (sizeof (UgtkSelectorItem));
		filter_item->uri  = key;
		filter_item->mark = TRUE;
		filter_item->data = NULL;
		g_ptr_array_add (filter_items, filter_item);
		filter_list = NULL;
	}
	else {
		g_hash_table_steal (page->filter.hash, key);
		g_free (key);
		key = orig_key;
	}
	filter_list = g_list_prepend (filter_list, value);
	g_hash_table_insert (page->filter.hash, key, filter_list);
}

void  ugtk_selector_page_make_filter (UgtkSelectorPage* page)
{
	UgtkSelectorItem* item;
	UgUri*  upart;
	int     value;
	gchar*  key;
	guint   i;

	if (g_hash_table_size (page->filter.hash))
		return;

	upart = g_slice_alloc (sizeof (UgUri));
	for (i = 0; i < page->items->len; i++) {
		item = g_ptr_array_index (page->items, i);
		// create filter by host
		ug_uri_init (upart, item->uri);
		if (upart->authority)
			key = g_strndup (item->uri, upart->path);
		else
			key = g_strdup ("(none)");
		ugtk_selector_page_add_filter (page, page->filter.host, key, item);
		// create filter by filename extension
		value = ug_uri_part_file_ext (upart, (const char**) &key);
		if (value)
			key = g_strdup_printf (".%.*s", value, key);
		else
			key = g_strdup (".(none)");
		ugtk_selector_page_add_filter (page, page->filter.ext, key, item);
	}
	g_slice_free1 (sizeof (UgUri), upart);
}

static void ugtk_selector_page_mark_by_filter (UgtkSelectorPage* page, GPtrArray* filter_items)
{
	UgtkSelectorItem*  item;
	GList*  related;
	GList*  marked;
	GList*  link;

	marked = ugtk_selector_items_get_marked (filter_items, NULL);
	for (link = marked;  link;  link = link->next) {
		item = link->data;
		related = g_hash_table_lookup (page->filter.hash, item->uri);
		for (;  related;  related = related->next) {
			item = related->data;
			item->mark++;	// increase mark count
		}
	}
	g_list_free (marked);
}

void  ugtk_selector_page_mark_by_filter_all (UgtkSelectorPage* page)
{
	UgtkSelectorItem*  item;
	guint i;

	// clear all mark
	ugtk_selector_items_set_mark_all (page->items, FALSE);
	page->n_marked = 0;
	// If filter (host and filename extension) was selected, increase mark count.
	ugtk_selector_page_mark_by_filter (page, page->filter.host);
	ugtk_selector_page_mark_by_filter (page, page->filter.ext);
	// remark
	for (i = 0; i < page->items->len; i++) {
		item = g_ptr_array_index (page->items, i);
		if (item->mark > 0) {
			item->mark--;
			if (item->mark)
				page->n_marked++;
		}
	}
}


// ----------------------------------------------------------------------------
// GPtrArray helpers for UgtkSelectorItem

static GList*  ugtk_selector_items_get_marked (GPtrArray* items, GList* list)
{
	guint i;
	for (i = 0; i < items->len; i++) {
		UgtkSelectorItem* item = g_ptr_array_index (items, i);
		if (item->mark)
			list = g_list_prepend (list, item);
	}
	return list;
}

static void ugtk_selector_items_set_mark_all (GPtrArray* items, gboolean mark)
{
	guint i;
	for (i = 0; i < items->len; i++) {
		UgtkSelectorItem* item = g_ptr_array_index (items, i);
		item->mark = mark;
	}
}

static void ugtk_selector_items_clear (GPtrArray* items)
{
	guint i;
	for (i = 0; i < items->len; i++) {
		UgtkSelectorItem* item = g_ptr_array_index (items, i);
		g_free (item->uri);
		g_slice_free1 (sizeof (UgtkSelectorItem), item);
	}
	g_ptr_array_set_size (items, 0);
}


// UI
static void ugtk_selector_init_ui (UgtkSelector* selector)
{
	GtkBox*     vbox;
	GtkBox*     hbox;
	GtkWidget*  widget;
	gchar*      string;

	selector->self = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	vbox = (GtkBox*) selector->self;

	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	string = g_strconcat (_("Base hypertext reference"), " <base href> :", NULL);
	selector->href_label = gtk_label_new (string);
	g_free (string);
	gtk_box_append (hbox, selector->href_label);

	selector->href_entry = (GtkEntry*) gtk_entry_new ();
	gtk_box_append (vbox, (GtkWidget*) selector->href_entry);
	selector->href_separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_box_append (vbox, selector->href_separator);

	selector->notebook = (GtkNotebook*) gtk_notebook_new ();
	gtk_box_append (vbox, (GtkWidget*) selector->notebook);

	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	// select all
	widget = gtk_button_new_with_mnemonic (_("Mark _All"));
	gtk_box_append (hbox, widget);
	selector->select_all = widget;
	// select none
	widget = gtk_button_new_with_mnemonic (_("Mark _None"));
	gtk_box_append (hbox, widget);
	selector->select_none = widget;
	// select by filter
	widget = gtk_button_new_with_mnemonic (_("_Mark by filter..."));
	gtk_box_append (hbox, widget);
	selector->select_filter = widget;

	gtk_widget_set_visible ((GtkWidget*) vbox, TRUE);
}
