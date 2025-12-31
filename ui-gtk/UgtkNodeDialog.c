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
#include <UgtkNodeDialog.h>

#include <glib/gi18n.h>

// UI
static void ugtk_node_dialog_init_ui (UgtkNodeDialog* ndialog,
                                      gboolean  has_category_form);
static void ugtk_node_dialog_init_list_ui (UgtkNodeDialog* ndialog,
                                           UgetNode* root);
// Callback
static void on_cursor_changed (GtkTreeView* view, UgtkNodeDialog* ndialog);
static void after_uri_entry_changed (GtkEditable *editable,
                                     UgtkNodeDialog* ndialog);
static void on_response_new_category (GtkDialog *dialog, gint response_id,
                                      UgtkNodeDialog* ndialog);
static void on_response_new_download (GtkDialog *dialog, gint response_id,
                                      UgtkNodeDialog* ndialog);
static void on_response_edit_category (GtkDialog *dialog, gint response_id,
                                       UgtkNodeDialog* ndialog);
static void on_response_edit_download (GtkDialog *dialog, gint response_id,
                                       UgtkNodeDialog* ndialog);

static void on_dialog_run_response (GtkDialog *dialog, gint response_id, gpointer user_data)
{
	gint *response = (gint*)user_data;
	*response = response_id;
}
// Callback for Main Window operate
static void on_category_row_changed (GtkTreeModel*   model,
                                     GtkTreePath*    path,
                                     GtkTreeIter*    iter,
                                     UgtkNodeDialog* ndialog);
static void on_category_row_deleted (GtkTreeModel*   model,
                                     GtkTreePath*    path,
                                     UgtkNodeDialog* ndialog);
static void on_category_row_inserted (GtkTreeModel*   model,
                                      GtkTreePath*    path,
                                      GtkTreeIter*    iter,
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
#if GTK_MAJOR_VERSION <= 3 && GTK_MINOR_VERSION < 14
	gtk_window_set_has_resize_grip (window, FALSE);
#endif

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
	gtk_dialog_add_button (ndialog->self, _("_Cancel"),
	                       GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (ndialog->self, _("_OK"),
	                       GTK_RESPONSE_OK);
	gtk_dialog_set_default_response (ndialog->self, GTK_RESPONSE_OK);

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
		g_signal_connect (ndialog->self, "response",
				G_CALLBACK (on_response_new_download), ndialog);
		break;

	case UGTK_NODE_DIALOG_NEW_CATEGORY:
		gtk_window_set_default_size ((GtkWindow*) ndialog->self, 300, 380);
		g_signal_connect (ndialog->self, "response",
				G_CALLBACK (on_response_new_category), ndialog);
		break;

	case UGTK_NODE_DIALOG_EDIT_DOWNLOAD:
		g_signal_connect (ndialog->self, "response",
				G_CALLBACK (on_response_edit_download), ndialog);
		break;

	case UGTK_NODE_DIALOG_EDIT_CATEGORY:
		gtk_window_set_default_size ((GtkWindow*) ndialog->self, 300, 380);
		g_signal_connect (ndialog->self, "response",
				G_CALLBACK (on_response_edit_category), ndialog);
		break;
	}

	ugtk_node_dialog_monitor_uri (ndialog);
//	gtk_dialog_run (ndialog->self);
	gtk_widget_show ((GtkWidget*) ndialog->self);

//	g_signal_connect (ndialog->button_back, "clicked",
//			G_CALLBACK (on_button_back), ndialog);
//	g_signal_connect (ndialog->button_forward, "clicked",
//			G_CALLBACK (on_button_forward), ndialog);
}

void  ugtk_node_dialog_monitor_uri (UgtkNodeDialog* ndialog)
{
	GtkEditable*  editable;

	if (gtk_widget_get_sensitive (ndialog->download.uri_entry)) {
		gtk_dialog_set_response_sensitive (ndialog->self,
				GTK_RESPONSE_OK, ndialog->download.completed);
		editable = GTK_EDITABLE (ndialog->download.uri_entry);
		g_signal_connect_after (editable, "changed",
				G_CALLBACK (after_uri_entry_changed), ndialog);
	}
}

gboolean  ugtk_node_dialog_confirm_existing (UgtkNodeDialog* ndialog, const char* uri)
{
	GtkWidget*  dialog;
	gboolean    existing;
	int         response;
	char*       title;

	existing = uget_uri_hash_find (ndialog->app->uri_hash, uri);
	if (existing) {
		dialog = gtk_message_dialog_new ((GtkWindow*) ndialog->self,
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
				_("URI had existed"));
		gtk_message_dialog_format_secondary_text ((GtkMessageDialog*) dialog,
				"%s", _("This URI had existed, are you sure to continue?"));
		// title
		title = g_strconcat ("uGet - ", _("URI had existed"), NULL);
		gtk_window_set_title ((GtkWindow*) dialog, title);
		g_free (title);
		// run and get response
		// run and get response
		// response = gtk_dialog_run (GTK_DIALOG (dialog));
		g_signal_connect (dialog, "response", G_CALLBACK (on_dialog_run_response), &response);
		gtk_widget_show (dialog);
		response = GTK_RESPONSE_NONE;
		while (response == GTK_RESPONSE_NONE)
			g_main_context_iteration (NULL, TRUE);

		gtk_window_destroy (GTK_WINDOW (dialog));
		if (response == GTK_RESPONSE_NO)
			return FALSE;
	}
	return TRUE;
}

void  ugtk_node_dialog_store_recent (UgtkNodeDialog* ndialog, UgtkApp* app)
{
	GtkTreePath*  path;
	int    nth;

	app->recent.saved = TRUE;
	gtk_tree_view_get_cursor (ndialog->node_view, &path, NULL);
	if (path != NULL) {
		nth = *gtk_tree_path_get_indices (path);
		app->recent.category_index = nth;
		gtk_tree_path_free (path);
	}
	ugtk_download_form_get(&ndialog->download, app->recent.info);
}

void  ugtk_node_dialog_apply_recent (UgtkNodeDialog* ndialog, UgtkApp* app)
{
	GtkTreePath*  path;

	if (app->recent.saved && app->setting.ui.apply_recent) {
		path = gtk_tree_path_new_from_indices (app->recent.category_index, -1);
		gtk_tree_view_set_cursor (ndialog->node_view, path, NULL, FALSE);
		gtk_tree_path_free (path);
		ndialog->download.changed.uri = TRUE;
		ugtk_download_form_set(&ndialog->download,
		                       app->recent.info, TRUE);
	}
}

void  ugtk_node_dialog_set_category (UgtkNodeDialog* ndialog, UgetNode* cnode)
{
	GtkTreeModel* model;
	GtkTreePath*  path;
	int           nth;

	if (cnode == NULL) {
		if (ndialog->node_tree == NULL)
			return;
		model = GTK_TREE_MODEL (ndialog->app->traveler.category.model);
		for (nth = 0;  nth < 3;  nth++)
			g_signal_handler_disconnect (model, ndialog->handler_id[nth]);
		return;
	}

	nth = uget_node_child_position (cnode->parent, cnode);
	ugtk_node_dialog_init_list_ui (ndialog, cnode->parent);
	g_signal_connect (ndialog->node_view, "cursor-changed",
			G_CALLBACK (on_cursor_changed), ndialog);
	path = gtk_tree_path_new_from_indices (nth, -1);
	gtk_tree_view_set_cursor (ndialog->node_view, path, NULL, FALSE);
	gtk_tree_path_free (path);
	// signal
	model = GTK_TREE_MODEL (ndialog->app->traveler.category.model);
	ndialog->handler_id[0] = g_signal_connect (model, "row-changed",
			G_CALLBACK (on_category_row_changed), ndialog);
	ndialog->handler_id[1] = g_signal_connect (model, "row-deleted",
			G_CALLBACK (on_category_row_deleted), ndialog);
	ndialog->handler_id[2] = g_signal_connect (model, "row-inserted",
			G_CALLBACK (on_category_row_inserted), ndialog);
}

int  ugtk_node_dialog_get_category (UgtkNodeDialog* ndialog, UgetNode** cnode)
{
	GtkTreePath*  path;
	int           nth;

	if (ndialog->node_tree == NULL) {
		*cnode = NULL;
		return -1;
	}
	gtk_tree_view_get_cursor (ndialog->node_view, &path, NULL);
	nth = *gtk_tree_path_get_indices (path);
	gtk_tree_path_free (path);

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
	GtkBox*       box;

	ndialog->self = (GtkDialog*) gtk_dialog_new ();

	// content
	box = (GtkBox*) gtk_dialog_get_content_area (ndialog->self);
	widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (box, widget);
	ndialog->hbox = (GtkBox*) widget;
	widget = gtk_notebook_new ();
	gtk_box_append (ndialog->hbox, widget);
	ndialog->notebook = widget;
	notebook = (GtkNotebook*) widget;
	gtk_widget_set_visible (GTK_WIDGET (box), TRUE);

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

//	gtk_widget_show (GTK_WIDGET (notebook));
}

static void ugtk_node_dialog_init_list_ui (UgtkNodeDialog* ndialog,
                                           UgetNode* root)
{
	GtkTreeModel* model;
	GtkWidget*    scrolled;
	GtkBox*       vbox;
	int           width;

	// decide width
	if (ndialog->app->setting.window.category)
		width = gtk_paned_get_position (ndialog->app->window.hpaned);
	else
		width = 165;

	ndialog->node_tree = ugtk_node_tree_new (root, TRUE);
	ndialog->node_view = (GtkTreeView*) ugtk_node_view_new_for_category ();
	model = GTK_TREE_MODEL (ndialog->node_tree);
	gtk_tree_view_set_model (ndialog->node_view, model);
	ugtk_node_view_use_large_icon (ndialog->node_view,
			ndialog->app->setting.ui.large_icon,
			ndialog->app->setting.download_column.width.state);

	scrolled = gtk_scrolled_window_new ();
	gtk_widget_set_size_request (scrolled, width, 200);
	gtk_widget_show (scrolled);
	// gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled),
			GTK_WIDGET (ndialog->node_view));
	// pack vbox
	vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	gtk_box_append (vbox, gtk_label_new (_("Category")));
	gtk_box_append (vbox, (GtkWidget*) scrolled);
	gtk_box_prepend (ndialog->hbox, (GtkWidget*) vbox);
	gtk_widget_set_visible ((GtkWidget*) vbox, TRUE);
}

// ----------------------------------------------------------------------------
// Callback

static void on_cursor_changed (GtkTreeView* view, UgtkNodeDialog* ndialog)
{
	GtkTreeModel*  model;
	GtkTreePath*   path;
	GtkTreeIter    iter;
	UgetNode*      node;

	// apply settings
	model = gtk_tree_view_get_model (view);
	gtk_tree_view_get_cursor (view, &path, NULL);
	if (path == NULL)
		return;
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);
	node = iter.user_data;
	ugtk_proxy_form_set(&ndialog->proxy, node->info, TRUE);
	ugtk_download_form_set(&ndialog->download, node->info, TRUE);
}

static void after_uri_entry_changed (GtkEditable *editable,
                                     UgtkNodeDialog* ndialog)
{
	gtk_dialog_set_response_sensitive (ndialog->self,
			GTK_RESPONSE_OK, ndialog->download.completed);
}

static void on_response_new_category (GtkDialog *dialog, gint response_id,
                                      UgtkNodeDialog* ndialog)
{
	UgetNode* cnode;

	if (response_id == GTK_RESPONSE_OK) {
		cnode = uget_node_new (NULL);
		ugtk_node_dialog_get(ndialog, cnode->info);
		uget_app_add_category ((UgetApp*) ndialog->app, cnode, TRUE);
		ugtk_app_decide_category_sensitive (ndialog->app);
		ugtk_download_form_get_folders (&ndialog->download,
		                                &ndialog->app->setting);
	}
	if (ndialog->node_info)
		ug_info_unref(ndialog->node_info);
	ugtk_node_dialog_free (ndialog);
}

static void on_response_new_download (GtkDialog *dialog, gint response_id,
                                      UgtkNodeDialog* ndialog)
{
	UgetNode*   cnode;
	UgetNode*   dnode;
	const char* uri;

	if (response_id == GTK_RESPONSE_OK) {
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
	}
	if (ndialog->node_info)
		ug_info_unref(ndialog->node_info);
	ugtk_node_dialog_free (ndialog);
}

static void on_response_edit_category (GtkDialog *dialog, gint response_id,
                                       UgtkNodeDialog* ndialog)
{
	UgtkApp* app;

	if (response_id == GTK_RESPONSE_OK && ndialog->node_info) {
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

static void on_response_edit_download (GtkDialog *dialog, gint response_id,
                                       UgtkNodeDialog* ndialog)
{
	UgtkApp*    app;

	if (response_id == GTK_RESPONSE_OK && ndialog->node_info) {
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

// ----------------------------------------------------------------------------
// Callback for Main Window operate

static void on_category_row_changed (GtkTreeModel*   model,
                                     GtkTreePath*    path_mw,
                                     GtkTreeIter*    iter_mw,
                                     UgtkNodeDialog* ndialog)
{
	GtkTreePath*  path;
	GtkTreeIter*  iter;

	path = gtk_tree_path_copy (path_mw);
	iter = gtk_tree_iter_copy (iter_mw);
	iter->stamp = ndialog->node_tree->stamp;
	gtk_tree_path_prev (path);
	gtk_tree_model_row_changed (GTK_TREE_MODEL (ndialog->node_tree),
	                            path, iter);
	gtk_tree_path_free (path);
	gtk_tree_iter_free (iter);
}

static void on_category_row_deleted (GtkTreeModel*   model,
                                     GtkTreePath*    path_mw,
                                     UgtkNodeDialog* ndialog)
{
	GtkTreePath*  path;

	path = gtk_tree_path_copy (path_mw);
	gtk_tree_path_prev (path);
	gtk_tree_model_row_deleted (GTK_TREE_MODEL (ndialog->node_tree), path);
	gtk_tree_path_free (path);
}

static void on_category_row_inserted (GtkTreeModel*   model,
                                      GtkTreePath*    path_mw,
                                      GtkTreeIter*    iter_mw,
                                      UgtkNodeDialog* ndialog)
{
	GtkTreePath*  path;
	GtkTreeIter*  iter;

	path = gtk_tree_path_copy (path_mw);
	iter = gtk_tree_iter_copy (iter_mw);
	iter->stamp = ndialog->node_tree->stamp;
	gtk_tree_path_prev (path);
	gtk_tree_model_row_inserted (GTK_TREE_MODEL (ndialog->node_tree),
	                             path, iter);
	gtk_tree_path_free (path);
	gtk_tree_iter_free (iter);
}
