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
#include <UgtkSummary.h>

#include <glib/gi18n.h>

// static functions
static GtkTreeView* ugtk_summary_view_new ();
static void         ugtk_summary_store_realloc_next (GtkListStore* store, GtkTreeIter* iter);
static void         ugtk_summary_menu_init (UgtkSummary* summary, gpointer accel_group);

void  ugtk_summary_init (UgtkSummary* summary, gpointer accel_group)
{
	GtkScrolledWindow*	scroll;

	summary->store = gtk_list_store_new (UGTK_SUMMARY_N_COLUMN,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	summary->view = ugtk_summary_view_new ();
	gtk_tree_view_set_model (summary->view, GTK_TREE_MODEL (summary->store));

	summary->self = gtk_scrolled_window_new ();
	scroll = GTK_SCROLLED_WINDOW (summary->self);
	// gtk_scrolled_window_set_shadow_type (scroll, GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (scroll,
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_child (scroll, GTK_WIDGET (summary->view));
	// gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (summary->view));
	gtk_widget_set_size_request (summary->self, 200, 90);
	gtk_widget_show (summary->self);
	// gtk_widget_show_all (summary->self);

	// menu
	ugtk_summary_menu_init (summary, accel_group);
	// visible
	summary->visible.name = 1;
	summary->visible.folder = 1;
	summary->visible.category = 0;
	summary->visible.uri = 0;
	summary->visible.message = 1;
}

void  ugtk_summary_show (UgtkSummary* summary, UgetNode* node)
{
	GtkTreeIter   iter;
	gchar*        name;
	gchar*        value;
	gchar*        stock;
	union {
		UgetLog*      log;
		UgetEvent*    event;
		UgetCommon*   common;
	} temp;

	if (node == NULL) {
		gtk_list_store_clear (summary->store);
		return;
	}

	iter.stamp = 0;   // used by ugtk_summary_store_realloc_next()
	temp.common = ug_info_get (node->info, UgetCommonInfo);

	// Summary Name
	if (summary->visible.name) {
		if (temp.common && temp.common->name) {
			name = g_strconcat (_("Name"), ":", NULL);
			value = temp.common->name;
		}
		else {
			name = g_strconcat (_("File"), ":", NULL);
			value = (temp.common) ? temp.common->file : NULL;
			if (value == NULL)
				value = _("unnamed");
		}
		ugtk_summary_store_realloc_next (summary->store, &iter);
		gtk_list_store_set (summary->store, &iter,
				UGTK_SUMMARY_COLUMN_ICON , "text-x-generic",
				UGTK_SUMMARY_COLUMN_NAME , name,
				UGTK_SUMMARY_COLUMN_VALUE, value,
				-1);
		g_free (name);
	}
	// Summary Folder
	if (summary->visible.folder) {
		name = g_strconcat (_("Folder"), ":", NULL);
		value = (temp.common) ? temp.common->folder : NULL;
		ugtk_summary_store_realloc_next (summary->store, &iter);
		gtk_list_store_set (summary->store, &iter,
				UGTK_SUMMARY_COLUMN_ICON , "folder",
				UGTK_SUMMARY_COLUMN_NAME , name,
				UGTK_SUMMARY_COLUMN_VALUE, value,
				-1);
		g_free (name);
	}
	// Summary Category
	if (summary->visible.category) {
		name = g_strconcat (_("Category"), ":", NULL);
		if (node->parent) {
			temp.common = ug_info_get (node->parent->info, UgetCommonInfo);
			value = (temp.common) ? temp.common->name : NULL;
			temp.common = ug_info_get (node->info, UgetCommonInfo);
		}
		else
			value = NULL;
		ugtk_summary_store_realloc_next (summary->store, &iter);
		gtk_list_store_set (summary->store, &iter,
				UGTK_SUMMARY_COLUMN_ICON , "gtk-dnd-multiple",
				UGTK_SUMMARY_COLUMN_NAME , name,
				UGTK_SUMMARY_COLUMN_VALUE, value,
				-1);
		g_free (name);
	}
	// Summary URL
	if (summary->visible.uri) {
		name = g_strconcat (_("URI"), ":", NULL);
		value = (temp.common) ? temp.common->uri : NULL;
		ugtk_summary_store_realloc_next (summary->store, &iter);
		gtk_list_store_set (summary->store, &iter,
				UGTK_SUMMARY_COLUMN_ICON , "network-workgroup",
				UGTK_SUMMARY_COLUMN_NAME , name,
				UGTK_SUMMARY_COLUMN_VALUE, value,
				-1);
		g_free (name);
	}
	// Summary Message
	temp.log = ug_info_get (node->info, UgetLogInfo);
	if (temp.log)
		temp.event = (UgetEvent*) temp.log->messages.head;
	if (summary->visible.message) {
		if (temp.event == NULL) {
			stock = "dialog-information";
			value = NULL;
		}
		else {
			value = temp.event->string;
			switch (temp.event->type) {
			case UGET_EVENT_ERROR:
				stock = "dialog-error";
				break;
			case UGET_EVENT_WARNING:
				stock = "dialog-warning";
				break;
			default:
				stock = "dialog-information";
				break;
			}
		}
		name = g_strconcat (_("Message"), ":", NULL);
		ugtk_summary_store_realloc_next (summary->store, &iter);
		gtk_list_store_set (summary->store, &iter,
				UGTK_SUMMARY_COLUMN_ICON , stock,
				UGTK_SUMMARY_COLUMN_NAME , name,
				UGTK_SUMMARY_COLUMN_VALUE, value,
				-1);
	}
	// clear remaining rows
	if (gtk_tree_model_iter_next (GTK_TREE_MODEL (summary->store), &iter)) {
		while (gtk_list_store_remove (summary->store, &iter))
			continue;
	}
}

gchar*  ugtk_summary_get_text_selected (UgtkSummary* summary)
{
	GtkTreeModel*		model;
	GtkTreePath*		path;
	GtkTreeIter			iter;
	gchar*				name;
	gchar*				value;

	gtk_tree_view_get_cursor (summary->view, &path, NULL);
	if (path == NULL)
		return NULL;

	model = GTK_TREE_MODEL (summary->store);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);
	gtk_tree_model_get (model, &iter,
			UGTK_SUMMARY_COLUMN_NAME , &name,
			UGTK_SUMMARY_COLUMN_VALUE, &value,
			-1);
	return g_strconcat (name, " ", value, NULL);
}

gchar*  ugtk_summary_get_text_all (UgtkSummary* summary)
{
	GtkTreeModel*		model;
	GtkTreeIter			iter;
	gchar*				name;
	gchar*				value;
	gboolean			valid;
	GString*			gstr;

	gstr  = g_string_sized_new (60);
	model = GTK_TREE_MODEL (summary->store);
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid) {
		gtk_tree_model_get (model, &iter,
				UGTK_SUMMARY_COLUMN_NAME , &name,
				UGTK_SUMMARY_COLUMN_VALUE, &value,
				-1);
		valid = gtk_tree_model_iter_next (model, &iter);
		// string
		g_string_append (gstr, name);
		if (value) {
			g_string_append_c (gstr, ' ');
			g_string_append (gstr, value);
		}
		g_string_append_c (gstr, '\n');
	}
	return g_string_free (gstr, FALSE);
}

// ----------------------------------------------------------------------------
// UgtkSummary static functions
//
static GtkTreeView* ugtk_summary_view_new ()
{
	GtkTreeView*		view;
	GtkCellRenderer*	renderer;

	view = (GtkTreeView*) gtk_tree_view_new ();
	gtk_tree_view_set_headers_visible (view, FALSE);
	// columns
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_insert_column_with_attributes (
			view, UGTK_SUMMARY_COLUMN_ICON,
			NULL, renderer,
			"icon-name", UGTK_SUMMARY_COLUMN_ICON,
			NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (
			view, UGTK_SUMMARY_COLUMN_NAME,
			_("Item"), renderer,
			"text", UGTK_SUMMARY_COLUMN_NAME,
			NULL);
	gtk_tree_view_insert_column_with_attributes (
			view, UGTK_SUMMARY_COLUMN_VALUE,
			_("Value"), renderer,
			"text", UGTK_SUMMARY_COLUMN_VALUE,
			NULL);
	return view;
}

static void ugtk_summary_store_realloc_next (GtkListStore* store, GtkTreeIter* iter)
{
	GtkTreeModel*	model;

	model = GTK_TREE_MODEL (store);
	if (iter->stamp == 0) {
		if (gtk_tree_model_get_iter_first (model, iter) == FALSE)
			gtk_list_store_append (store, iter);
	}
	else if (gtk_tree_model_iter_next (model, iter) == FALSE)
		gtk_list_store_append (store, iter);
}

static void ugtk_summary_menu_init (UgtkSummary* summary, gpointer accel_group)
{
	/*
	GtkWidget*		image;
	GtkWidget*		menu;
	GtkWidget*		menu_item;

	// UgtkSummary.menu
	menu = gtk_menu_new ();
	// Copy
	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_COPY, accel_group);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	summary->menu.copy = menu_item;
	// Copy All
	menu_item = gtk_image_menu_item_new_with_mnemonic (_("Copy _All"));
	image = gtk_image_new_from_stock (GTK_STOCK_SELECT_ALL, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	summary->menu.copy_all = menu_item;

	gtk_widget_show_all (menu);
	summary->menu.self = GTK_MENU (menu);
	*/
}

