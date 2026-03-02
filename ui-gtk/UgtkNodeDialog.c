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

#include <UgtkNodeView.h>
#include <UgtkNodeObject.h>
#include <UgtkNodeDialog.h>

#include <glib/gi18n.h>

// UI
static void ugtk_node_dialog_init_ui (UgtkNodeDialog* ndialog,
                                      gboolean  has_category_form);
static void ugtk_node_dialog_init_list_ui (UgtkNodeDialog* ndialog,
                                           UgetNode* root);
// Callback
static void on_selection_changed (GtkSingleSelection* sel, GParamSpec* pspec,
                                  UgtkNodeDialog* ndialog);
static void after_uri_entry_changed (GtkEditable *editable,
                                     UgtkNodeDialog* ndialog);
static void on_ok_new_category (GtkWidget* button, UgtkNodeDialog* ndialog);
static void on_ok_new_download (GtkWidget* button, UgtkNodeDialog* ndialog);
static void on_ok_edit_category (GtkWidget* button, UgtkNodeDialog* ndialog);
static void on_ok_edit_download (GtkWidget* button, UgtkNodeDialog* ndialog);
static void on_cancel_node_dialog (GtkWidget* button, UgtkNodeDialog* ndialog);
static gboolean on_close_node_dialog (GtkWindow* window, UgtkNodeDialog* ndialog);

// Callback for Main Window model changes
static void on_category_items_changed (GListModel* model, guint pos,
                                       guint removed, guint added,
                                       UgtkNodeDialog* ndialog);

// ----------------------------------------------------------------------------
// UgtkNodeDialog

void  ugtk_node_dialog_init (UgtkNodeDialog* ndialog,
                             const char*     title,
                             UgtkApp*        app,
                             gboolean        has_category_form)
{
	GtkWindow*  window;
	int         sensitive;
	int         width, height, temp;

	ugtk_node_dialog_init_ui (ndialog, has_category_form);
	ndialog->app = app;

	// decide width
	if (app->setting.window.category) {
		gtk_widget_get_size_request (ndialog->notebook, &width, &height);
		temp = gtk_paned_get_position (ndialog->app->window.hpaned);
		temp = temp * 5 / 3;  // (temp * 1.666)
		if (width < temp)
			gtk_widget_set_size_request (ndialog->notebook, temp, height);
	}

	window = (GtkWindow*) ndialog->self;
	gtk_window_set_transient_for (window, app->window.self);
	gtk_window_set_destroy_with_parent (window, TRUE);
	if (title)
		gtk_window_set_title (window, title);
	// decide sensitive by plug-in matching order
	switch (app->setting.plugin_order) {
	default:
	case UGTK_PLUGIN_ORDER_ARIA2:
	case UGTK_PLUGIN_ORDER_ARIA2_CURL:
		sensitive = FALSE;
		break;

	case UGTK_PLUGIN_ORDER_CURL:
	case UGTK_PLUGIN_ORDER_CURL_ARIA2:
		sensitive = TRUE;
		break;
	}

	gtk_widget_set_sensitive ((GtkWidget*) ndialog->download.cookie_label, sensitive);
	gtk_widget_set_sensitive ((GtkWidget*) ndialog->download.cookie_entry, sensitive);
	gtk_widget_set_sensitive ((GtkWidget*) ndialog->download.post_label, sensitive);
	gtk_widget_set_sensitive ((GtkWidget*) ndialog->download.post_entry, sensitive);
}

UgtkNodeDialog*  ugtk_node_dialog_new (const char* title,
                                       UgtkApp*    app,
                                       gboolean    has_category_form)
{
	UgtkNodeDialog*  ndialog;

	ndialog = g_malloc0 (sizeof (UgtkNodeDialog));
	ugtk_node_dialog_init (ndialog, title, app, has_category_form);
	// OK & cancel buttons
	ndialog->cancel_button = gtk_button_new_with_mnemonic (_("_Cancel"));
	gtk_box_append (ndialog->button_box, ndialog->cancel_button);
	ndialog->ok_button = gtk_button_new_with_mnemonic (_("_OK"));
	gtk_widget_add_css_class (ndialog->ok_button, "suggested-action");
	gtk_box_append (ndialog->button_box, ndialog->ok_button);

	return ndialog;
}

void  ugtk_node_dialog_free (UgtkNodeDialog* ndialog)
{
	ugtk_node_dialog_set_category (ndialog, NULL);
	gtk_window_destroy (GTK_WINDOW (ndialog->self));
	g_free (ndialog);
}

void  ugtk_node_dialog_run (UgtkNodeDialog* ndialog,
                            UgtkNodeDialogMode mode,
                            UgetNode*          node)
{
	if (node) {
		ndialog->node = node;
		ndialog->node_info = node->info;
		ug_info_ref(node->info);
	}

	switch (mode) {
	case UGTK_NODE_DIALOG_NEW_DOWNLOAD:
		ugtk_node_dialog_apply_recent (ndialog, ndialog->app);
		g_signal_connect (ndialog->ok_button, "clicked",
				G_CALLBACK (on_ok_new_download), ndialog);
		break;

	case UGTK_NODE_DIALOG_NEW_CATEGORY:
		gtk_window_set_default_size (ndialog->self, 300, 380);
		g_signal_connect (ndialog->ok_button, "clicked",
				G_CALLBACK (on_ok_new_category), ndialog);
		break;

	case UGTK_NODE_DIALOG_EDIT_DOWNLOAD:
		g_signal_connect (ndialog->ok_button, "clicked",
				G_CALLBACK (on_ok_edit_download), ndialog);
		break;

	case UGTK_NODE_DIALOG_EDIT_CATEGORY:
		gtk_window_set_default_size (ndialog->self, 300, 380);
		g_signal_connect (ndialog->ok_button, "clicked",
				G_CALLBACK (on_ok_edit_category), ndialog);
		break;
	}

	g_signal_connect (ndialog->cancel_button, "clicked",
			G_CALLBACK (on_cancel_node_dialog), ndialog);
	g_signal_connect (ndialog->self, "close-request",
			G_CALLBACK (on_close_node_dialog), ndialog);
	ugtk_node_dialog_monitor_uri (ndialog);
	gtk_widget_set_visible((GtkWidget*) ndialog->self, TRUE);
}

void  ugtk_node_dialog_monitor_uri (UgtkNodeDialog* ndialog)
{
	GtkEditable*  editable;

	if (gtk_widget_get_sensitive (ndialog->download.uri_entry)) {
		gtk_widget_set_sensitive (ndialog->ok_button,
				ndialog->download.completed);
		editable = GTK_EDITABLE (ndialog->download.uri_entry);
		g_signal_connect_after (editable, "changed",
				G_CALLBACK (after_uri_entry_changed), ndialog);
	}
}

static void on_confirm_existing_response (GObject* source, GAsyncResult* result, gpointer user_data)
{
	int *choice = (int*) user_data;
	*choice = gtk_alert_dialog_choose_finish (GTK_ALERT_DIALOG (source), result, NULL);
}

gboolean  ugtk_node_dialog_confirm_existing (UgtkNodeDialog* ndialog, const char* uri)
{
	gboolean    existing;

	existing = uget_uri_hash_find (ndialog->app->uri_hash, uri);
	if (existing) {
		GtkAlertDialog* alert;
		int choice = -1;

		alert = gtk_alert_dialog_new ("%s", _("URI had existed"));
		gtk_alert_dialog_set_detail (alert,
				_("This URI had existed, are you sure to continue?"));
		gtk_alert_dialog_set_buttons (alert,
				(const char*[]){ _("_No"), _("_Yes"), NULL });
		gtk_alert_dialog_set_cancel_button (alert, 0);
		gtk_alert_dialog_set_default_button (alert, 1);
		gtk_alert_dialog_choose (alert, (GtkWindow*) ndialog->self, NULL,
				on_confirm_existing_response, &choice);
		while (choice == -1)
			g_main_context_iteration (NULL, TRUE);
		g_object_unref (alert);
		if (choice != 1)
			return FALSE;
	}
	return TRUE;
}

void  ugtk_node_dialog_store_recent (UgtkNodeDialog* ndialog, UgtkApp* app)
{
	GtkColumnView* cv;
	GtkSelectionModel* sel;
	GtkBitset* bitset;

	app->recent.saved = TRUE;
	cv = GTK_COLUMN_VIEW (ndialog->node_view);
	sel = gtk_column_view_get_model (cv);
	if (sel) {
		bitset = gtk_selection_model_get_selection (sel);
		if (gtk_bitset_get_size (bitset) > 0)
			app->recent.category_index = gtk_bitset_get_nth (bitset, 0);
		gtk_bitset_unref (bitset);
	}
	ugtk_download_form_get(&ndialog->download, app->recent.info);
}

void  ugtk_node_dialog_apply_recent (UgtkNodeDialog* ndialog, UgtkApp* app)
{
	GtkColumnView* cv;
	GtkSelectionModel* sel;

	if (app->recent.saved && app->setting.ui.apply_recent) {
		cv = GTK_COLUMN_VIEW (ndialog->node_view);
		sel = gtk_column_view_get_model (cv);
		if (sel)
			gtk_single_selection_set_selected (GTK_SINGLE_SELECTION (sel),
					app->recent.category_index);
		ndialog->download.changed.uri = TRUE;
		ugtk_download_form_set(&ndialog->download,
		                       app->recent.info, TRUE);
	}
}

void  ugtk_node_dialog_set_category (UgtkNodeDialog* ndialog, UgetNode* cnode)
{
	GListModel* model;
	int         nth;

	if (cnode == NULL) {
		if (ndialog->node_tree == NULL)
			return;
		model = G_LIST_MODEL (ndialog->app->traveler.category.model);
		g_signal_handler_disconnect (model, ndialog->handler_id[0]);
		return;
	}

	nth = uget_node_child_position (cnode->parent, cnode);
	ugtk_node_dialog_init_list_ui (ndialog, cnode->parent);

	// Set selection
	GtkColumnView* cv = GTK_COLUMN_VIEW (ndialog->node_view);
	GtkSelectionModel* sel = gtk_column_view_get_model (cv);
	if (sel) {
		gtk_single_selection_set_selected (GTK_SINGLE_SELECTION (sel), nth);
		g_signal_connect (sel, "notify::selected",
				G_CALLBACK (on_selection_changed), ndialog);
	}

	// Connect to main window's category model for sync
	model = G_LIST_MODEL (ndialog->app->traveler.category.model);
	ndialog->handler_id[0] = g_signal_connect (model, "items-changed",
			G_CALLBACK (on_category_items_changed), ndialog);
}

int  ugtk_node_dialog_get_category (UgtkNodeDialog* ndialog, UgetNode** cnode)
{
	GtkColumnView* cv;
	GtkSelectionModel* sel;
	guint nth;

	if (ndialog->node_tree == NULL) {
		*cnode = NULL;
		return -1;
	}
	cv = GTK_COLUMN_VIEW (ndialog->node_view);
	sel = gtk_column_view_get_model (cv);
	nth = gtk_single_selection_get_selected (GTK_SINGLE_SELECTION (sel));

	*cnode = uget_node_nth_child (ndialog->node_tree->root, nth);
	return nth;
}

void  ugtk_node_dialog_set (UgtkNodeDialog* ndialog, UgInfo* node_info)
{
	ugtk_proxy_form_set(&ndialog->proxy, node_info, FALSE);
	ugtk_download_form_set(&ndialog->download, node_info, FALSE);
	if (ndialog->category.self)
		ugtk_category_form_set(&ndialog->category, node_info);
}

void  ugtk_node_dialog_get (UgtkNodeDialog* ndialog, UgInfo* node_info)
{
	ugtk_proxy_form_get(&ndialog->proxy, node_info);
	ugtk_download_form_get(&ndialog->download, node_info);
	if (ndialog->category.self)
		ugtk_category_form_get(&ndialog->category, node_info);
}

// ----------------------------------------------------------------------------
// UI
static void ugtk_node_dialog_init_ui (UgtkNodeDialog* ndialog,
                                      gboolean  has_category_form)
{
	GtkNotebook*  notebook;
	GtkWidget*    widget;
	GtkBox*       vbox;

	ndialog->self = (GtkWindow*) gtk_window_new ();

	// main vertical layout
	vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	gtk_window_set_child (ndialog->self, (GtkWidget*) vbox);

	// content area
	widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_widget_set_vexpand (widget, TRUE);
	gtk_box_append (vbox, widget);
	ndialog->hbox = (GtkBox*) widget;
	widget = gtk_notebook_new ();
	gtk_box_append (ndialog->hbox, widget);
	ndialog->notebook = widget;
	notebook = (GtkNotebook*) widget;

	// button box at bottom
	ndialog->button_box = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_set_halign ((GtkWidget*) ndialog->button_box, GTK_ALIGN_END);
	gtk_widget_set_margin_top ((GtkWidget*) ndialog->button_box, 6);
	gtk_widget_set_margin_bottom ((GtkWidget*) ndialog->button_box, 6);
	gtk_widget_set_margin_end ((GtkWidget*) ndialog->button_box, 6);
	gtk_box_append (vbox, (GtkWidget*) ndialog->button_box);

	// Download form (Page 1, 2)
	ugtk_proxy_form_init (&ndialog->proxy);
	ugtk_download_form_init (&ndialog->download,
			&ndialog->proxy, (GtkWindow*) ndialog->self);

	if (has_category_form == FALSE) {
		// UGTK_NODE_DIALOG_DOWNLOAD
		gtk_notebook_append_page (notebook, ndialog->download.page1,
				gtk_label_new (_("General")));
		gtk_notebook_append_page (notebook, ndialog->download.page2,
				gtk_label_new (_("Advanced")));
		// set focus widget
		gtk_window_set_focus (GTK_WINDOW (ndialog->self),
				ndialog->download.uri_entry);
	}
	else {
		// UGTK_NODE_DIALOG_CATEGORY
		ugtk_category_form_init (&ndialog->category);
		gtk_notebook_append_page (notebook, ndialog->category.self,
				gtk_label_new (_("Category settings")));
		gtk_notebook_append_page (notebook, ndialog->download.page1,
				gtk_label_new (_("Default for new download 1")));
		gtk_notebook_append_page (notebook, ndialog->download.page2,
				gtk_label_new (_("Default 2")));
		// hide field URI, mirrors, and rename
		ugtk_download_form_set_multiple (&ndialog->download, TRUE);
		// set focus widget
		gtk_window_set_focus (GTK_WINDOW (ndialog->self),
				ndialog->category.name_entry);
	}

}

static void ugtk_node_dialog_init_list_ui (UgtkNodeDialog* ndialog,
                                           UgetNode* root)
{
	GtkWidget*          scrolled;
	GtkBox*             vbox;
	GtkSingleSelection* sel;
	int                 width;

	// decide width
	if (ndialog->app->setting.window.category)
		width = gtk_paned_get_position (ndialog->app->window.hpaned);
	else
		width = 165;

	ndialog->node_tree = ugtk_node_tree_new (root, TRUE);
	ndialog->node_view = ugtk_node_view_new_for_category ();

	sel = gtk_single_selection_new (G_LIST_MODEL (ndialog->node_tree));
	gtk_column_view_set_model (GTK_COLUMN_VIEW (ndialog->node_view),
			GTK_SELECTION_MODEL (sel));

	scrolled = gtk_scrolled_window_new ();
	gtk_widget_set_size_request (scrolled, width, 200);
	gtk_widget_set_visible(scrolled, TRUE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled),
			ndialog->node_view);
	// pack vbox
	vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	gtk_box_append (vbox, gtk_label_new (_("Category")));
	gtk_box_append (vbox, (GtkWidget*) scrolled);
	gtk_box_prepend (ndialog->hbox, (GtkWidget*) vbox);
	gtk_widget_set_visible ((GtkWidget*) vbox, TRUE);
}

// ----------------------------------------------------------------------------
// Callback

static void on_selection_changed (GtkSingleSelection* sel, GParamSpec* pspec,
                                  UgtkNodeDialog* ndialog)
{
	UgtkNodeObject* obj;
	UgetNode*       node;
	guint           pos;

	pos = gtk_single_selection_get_selected (sel);
	if (pos == GTK_INVALID_LIST_POSITION)
		return;
	obj = g_list_model_get_item (G_LIST_MODEL (ndialog->node_tree), pos);
	if (obj == NULL)
		return;
	node = obj->node;
	g_object_unref (obj);
	ugtk_proxy_form_set(&ndialog->proxy, node->info, TRUE);
	ugtk_download_form_set(&ndialog->download, node->info, TRUE);
}

static void after_uri_entry_changed (GtkEditable *editable,
                                     UgtkNodeDialog* ndialog)
{
	gtk_widget_set_sensitive (ndialog->ok_button,
			ndialog->download.completed);
}

static void on_ok_new_category (GtkWidget* button, UgtkNodeDialog* ndialog)
{
	UgetNode* cnode;

	cnode = uget_node_new (NULL);
	ugtk_node_dialog_get(ndialog, cnode->info);
	uget_app_add_category ((UgetApp*) ndialog->app, cnode, TRUE);
	ugtk_app_decide_category_sensitive (ndialog->app);
	ugtk_download_form_get_folders (&ndialog->download,
	                                &ndialog->app->setting);
	if (ndialog->node_info)
		ug_info_unref(ndialog->node_info);
	ugtk_node_dialog_free (ndialog);
}

static void on_ok_new_download (GtkWidget* button, UgtkNodeDialog* ndialog)
{
	UgetNode*   cnode;
	UgetNode*   dnode;
	const char* uri;

	ugtk_node_dialog_store_recent (ndialog, ndialog->app);
	dnode = uget_node_new (NULL);
	ugtk_node_dialog_get(ndialog, dnode->info);
	ugtk_node_dialog_get_category (ndialog, &cnode);
	uri = gtk_editable_get_text ((GtkEditable*) ndialog->download.uri_entry);
	if (ugtk_node_dialog_confirm_existing (ndialog, uri)) {
		uget_app_add_download ((UgetApp*) ndialog->app, dnode, cnode, FALSE);
		ugtk_download_form_get_folders (&ndialog->download,
		                                &ndialog->app->setting);
	}
	if (ndialog->node_info)
		ug_info_unref(ndialog->node_info);
	ugtk_node_dialog_free (ndialog);
}

static void on_ok_edit_category (GtkWidget* button, UgtkNodeDialog* ndialog)
{
	UgtkApp* app;

	if (ndialog->node_info) {
		app = ndialog->app;
		ugtk_node_dialog_get(ndialog, ndialog->node_info);
		// if ndialog->node_info->ref_count == 1, ndialog->node is freed by App
		if (ndialog->node_info->ref_count > 1)
			ugtk_app_category_changed(app, ndialog->node);
		ug_info_unref(ndialog->node_info);
		ugtk_download_form_get_folders (&ndialog->download,
		                                &app->setting);
	}
	ugtk_node_dialog_free (ndialog);
}

static void on_ok_edit_download (GtkWidget* button, UgtkNodeDialog* ndialog)
{
	UgtkApp*    app;

	if (ndialog->node_info) {
		app = ndialog->app;
		uget_uri_hash_remove_download(app->uri_hash, ndialog->node_info);
		ugtk_node_dialog_get(ndialog, ndialog->node_info);
		uget_uri_hash_add_download(app->uri_hash, ndialog->node_info);
		// if ndialog->node_info->ref_count == 1, ndialog->node is freed by App
		if (ndialog->node_info->ref_count > 1) {
			ugtk_traveler_reserve_selection (&app->traveler);
			uget_app_reset_download_name((UgetApp*) app, ndialog->node);
			ugtk_traveler_restore_selection (&app->traveler);
		}
		ug_info_unref(ndialog->node_info);
		ugtk_download_form_get_folders (&ndialog->download,
		                                &app->setting);
	}
	ugtk_node_dialog_free (ndialog);
}

static void on_cancel_node_dialog (GtkWidget* button, UgtkNodeDialog* ndialog)
{
	if (ndialog->node_info)
		ug_info_unref(ndialog->node_info);
	ugtk_node_dialog_free (ndialog);
}

static gboolean on_close_node_dialog (GtkWindow* window, UgtkNodeDialog* ndialog)
{
	if (ndialog->node_info)
		ug_info_unref(ndialog->node_info);
	ugtk_node_dialog_free (ndialog);
	return TRUE;
}

// ----------------------------------------------------------------------------
// Callback for Main Window operate

static void on_category_items_changed (GListModel* model, guint pos,
                                       guint removed, guint added,
                                       UgtkNodeDialog* ndialog)
{
	// When the main window's category model changes, refresh our dialog's model
	ugtk_node_tree_refresh (ndialog->node_tree);
}
