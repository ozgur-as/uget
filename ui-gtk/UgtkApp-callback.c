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

#include <UgtkApp.h>
#include <UgtkTrayIcon.h>
#include <UgtkConfirmDialog.h>

// static functions
static void  ugtk_window_init_callback   (struct UgtkWindow*  window,  UgtkApp* app);
static void  ugtk_toolbar_init_callback  (struct UgtkToolbar* toolbar, UgtkApp* app);
// functions for UgetNode.notification
static void  node_inserted (UgetNode* node, UgetNode* sibling, UgetNode* child);
static void  node_removed (UgetNode* node, UgetNode* sibling, UgetNode* child);
static void  node_updated (UgetNode* child);

// GTK4 Helper for shortcuts
static void ugtk_add_shortcut (GtkWidget *widget, const char *accel, GtkShortcutFunc func, gpointer user_data) {
    GtkEventController *controller = gtk_shortcut_controller_new ();
    gtk_shortcut_controller_set_scope (GTK_SHORTCUT_CONTROLLER (controller), GTK_SHORTCUT_SCOPE_GLOBAL);
    
    GtkShortcutTrigger *trigger = gtk_shortcut_trigger_parse_string (accel);
    GtkShortcutAction *action = gtk_callback_action_new (func, user_data, NULL);
    GtkShortcut *shortcut = gtk_shortcut_new (trigger, action);
    
    gtk_shortcut_controller_add_shortcut (GTK_SHORTCUT_CONTROLLER (controller), shortcut);
    gtk_widget_add_controller (widget, controller);
}

// Shortcut Callbacks
static gboolean on_shortcut_delete (GtkWidget *widget, GVariant *args, gpointer user_data) {
    UgtkApp *app = (UgtkApp *)user_data;
    ugtk_app_delete_download (app, FALSE);
    return TRUE;
}

static gboolean on_shortcut_space (GtkWidget *widget, GVariant *args, gpointer user_data) {
    UgtkApp *app = (UgtkApp *)user_data;
    ugtk_app_switch_download_state (app);
    return TRUE;
}

static gboolean on_shortcut_new (GtkWidget *widget, GVariant *args, gpointer user_data) {
    UgtkApp *app = (UgtkApp *)user_data;
    // Assuming create download dialog
    ugtk_app_create_download (app, NULL, NULL);
    return TRUE;
}

static gboolean on_shortcut_quit (GtkWidget *widget, GVariant *args, gpointer user_data) {
    UgtkApp *app = (UgtkApp *)user_data;
    ugtk_app_quit (app);
    return TRUE;
}

void  ugtk_app_init_callback (UgtkApp* app)
{
//	gtk_accel_group_connect (app->accel_group, GDK_KEY_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE,
//	                         g_cclosure_new_swap (G_CALLBACK (ugtk_app_quit), app, NULL));
//	gtk_accel_group_connect (app->accel_group, GDK_KEY_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE,
//	                         g_cclosure_new_swap (G_CALLBACK (ugtk_app_save), app, NULL));
//	gtk_accel_group_connect (app->accel_group, GDK_KEY_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE,
//	                         g_cclosure_new_swap (G_CALLBACK (on_summary_copy_selected), app, NULL));
	ugtk_window_init_callback  (&app->window,  app);
	ugtk_menubar_init_callback (&app->menubar, app);
	ugtk_toolbar_init_callback (&app->toolbar, app);
	// Tray icon callbacks are set up in ugtk_tray_icon_init()
	// node notification
	uget_app_set_notification ((UgetApp*) app, app,
			node_inserted, node_removed, (UgNotifyFunc) node_updated);
}

// ----------------------------------------------------------------------------
// Toolbar

static void  on_create_download (GtkWidget* widget, UgtkApp* app)
{
	ugtk_app_create_download (app, NULL, NULL);
}

static void  on_set_download_runnable (GtkWidget* widget, UgtkApp* app)
{
	ugtk_app_queue_download (app, TRUE);
}

// ----------------------------------------------------------------------------
// UgtkWindow

// UgtkTraveler.download.selection "selection-changed"
static void  on_download_selection_changed (GtkSelectionModel* model, guint pos, guint n_items, UgtkApp* app)
{
	GtkBitset* bitset;
	guint64    n_selected;

	bitset = gtk_selection_model_get_selection (model);
	n_selected = gtk_bitset_get_size (bitset);
	gtk_bitset_unref (bitset);

	if (n_selected == 0)
		ugtk_summary_show (&app->summary, NULL);

	ugtk_statusbar_set_info (&app->statusbar, (gint) n_selected);
	ugtk_app_decide_download_sensitive (app);
}

// UgtkTraveler.download.selection "selection-changed" (cursor tracking)
static void  on_download_cursor_changed (GtkSelectionModel* model, guint pos, guint n_items, UgtkApp* app)
{
	UgetNode*     node;
	UgetRelation* relation;
	UgetPriority  priority;

	node = app->traveler.download.cursor.node;
	if (node)
		node = node->base;
	ugtk_summary_show (&app->summary, node);

	// UgtkMenubar.download.priority
	if (node == NULL)
		priority = UGET_PRIORITY_NORMAL;
	else {
		relation = ug_info_get (node->info, UgetRelationInfo);
		if (relation)
			priority = relation->priority;
		else
			priority = UGET_PRIORITY_NORMAL;
	}
	// Update GAction state for priority menu
	{
		GAction *paction = g_action_map_lookup_action (
			G_ACTION_MAP (app->action_group), "priority");
		if (paction) {
			const gchar *priority_str = "normal";
			if (priority == UGET_PRIORITY_HIGH)
				priority_str = "high";
			else if (priority == UGET_PRIORITY_LOW)
				priority_str = "low";
			g_simple_action_set_state (G_SIMPLE_ACTION (paction),
			                           g_variant_new_string (priority_str));
		}
	}
	
	// Update download action sensitivity - disable all except New when no download selected
	gboolean has_download = (app->traveler.download.cursor.node != NULL);
	GAction *action;
	
	action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "download-delete");
	if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_download);
	
	action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "download-delete-file");
	if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_download);
	
	action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "download-open");
	if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_download);
	
	action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "download-open-folder");
	if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_download);
	
	action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "download-force-start");
	if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_download);
	
	action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "download-start");
	if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_download);
	
	action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "download-pause");
	if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_download);
	
	action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "download-move-up");
	if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_download);
	
	action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "download-move-down");
	if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_download);
	
	action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "download-move-top");
	if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_download);
	
	action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "download-move-bottom");
	if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_download);
	
	action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "download-properties");
	if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_download);
	
	action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "priority");
	if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_download);
	
	action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "move-to-category");
	if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_download);
}

// UgtkTraveler.category/state.selection "selection-changed"
static void  on_category_cursor_changed (GtkSelectionModel* model, guint pos, guint n_items, UgtkApp* app)
{
	UgtkTraveler*  traveler;

	traveler = &app->traveler;
	if (traveler->state.cursor.pos    != traveler->state.cursor.pos_last ||
	    traveler->category.cursor.pos != traveler->category.cursor.pos_last)
	{
		// set sorting column
		ugtk_traveler_set_sorting (traveler, TRUE,
				app->setting.download_column.sort.nth,
				app->setting.download_column.sort.type);
	}

	// show/hide download column and setup items in UgtkViewMenu
	if (traveler->state.cursor.pos != traveler->state.cursor.pos_last) {
		struct UgtkDownloadColumnSetting*  setting;
		GtkColumnView*       view;
		GtkColumnViewColumn* column;
		GListModel*          columns;
		gboolean             sensitive;

		view = traveler->download.view;
		columns = gtk_column_view_get_columns (view);
		setting = &app->setting.download_column;
		// Finished
		if (traveler->state.cursor.pos == 3)
			sensitive = FALSE;
		else
			sensitive = TRUE;
		gtk_widget_set_sensitive (app->menubar.view.columns.complete, sensitive);
		gtk_widget_set_sensitive (app->menubar.view.columns.percent, sensitive);
		column = g_list_model_get_item (columns, UGTK_NODE_COLUMN_COMPLETE);
		gtk_column_view_column_set_visible (column, sensitive && setting->complete);
		g_object_unref (column);
		column = g_list_model_get_item (columns, UGTK_NODE_COLUMN_PERCENT);
		gtk_column_view_column_set_visible (column, sensitive && setting->percent);
		g_object_unref (column);
		// Recycled
		if (traveler->state.cursor.pos == 4)
			sensitive = FALSE;
		else
			sensitive = TRUE;
		gtk_widget_set_sensitive (app->menubar.view.columns.elapsed, sensitive);
		column = g_list_model_get_item (columns, UGTK_NODE_COLUMN_ELAPSED);
		gtk_column_view_column_set_visible (column, sensitive && setting->elapsed);
		g_object_unref (column);
		// Finished & Recycled
		if (traveler->state.cursor.pos == 3  ||  traveler->state.cursor.pos == 4)
			sensitive = FALSE;
		else
			sensitive = TRUE;
		gtk_widget_set_sensitive (app->menubar.view.columns.left, sensitive);
		gtk_widget_set_sensitive (app->menubar.view.columns.speed, sensitive);
		gtk_widget_set_sensitive (app->menubar.view.columns.upload_speed, sensitive);
		gtk_widget_set_sensitive (app->menubar.view.columns.uploaded, sensitive);
		gtk_widget_set_sensitive (app->menubar.view.columns.ratio, sensitive);
		column = g_list_model_get_item (columns, UGTK_NODE_COLUMN_LEFT);
		gtk_column_view_column_set_visible (column, sensitive && setting->left);
		g_object_unref (column);
		column = g_list_model_get_item (columns, UGTK_NODE_COLUMN_SPEED);
		gtk_column_view_column_set_visible (column, sensitive && setting->speed);
		g_object_unref (column);
		column = g_list_model_get_item (columns, UGTK_NODE_COLUMN_UPLOAD_SPEED);
		gtk_column_view_column_set_visible (column, sensitive && setting->upload_speed);
		g_object_unref (column);
		column = g_list_model_get_item (columns, UGTK_NODE_COLUMN_UPLOADED);
		gtk_column_view_column_set_visible (column, sensitive && setting->uploaded);
		g_object_unref (column);
		column = g_list_model_get_item (columns, UGTK_NODE_COLUMN_RATIO);
		gtk_column_view_column_set_visible (column, sensitive && setting->ratio);
		g_object_unref (column);
	}

	if (model == GTK_SELECTION_MODEL (app->traveler.category.selection)) {
		ugtk_app_decide_category_sensitive (app);
		// sync UgtkMenubar.download.move_to
		ugtk_menubar_sync_category (&app->menubar, app, FALSE);

		// Update category action sensitivity
		gboolean is_all_category = (app->traveler.category.cursor.pos == 0);
		gboolean is_first_category = (app->traveler.category.cursor.pos <= 1);
		gboolean is_last_category = (app->traveler.category.cursor.node &&
		                              app->traveler.category.cursor.node->next == NULL);
		GAction *action;

		action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "category-delete");
		if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), !is_all_category);

		action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "category-move-up");
		if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), !is_all_category && !is_first_category);

		action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "category-move-down");
		if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), !is_all_category && !is_last_category);

		action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "category-properties");
		if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), !is_all_category);
	}
}

// UgtkWindow.self "close-request" (GTK4)
static gboolean  on_window_close_request (GtkWindow* window, UgtkApp* app)
{
	if (app->setting.ui.close_to_tray == FALSE)
		ugtk_app_decide_to_quit (app);
	else {
		ugtk_tray_icon_set_visible (&app->trayicon, TRUE);
		gtk_widget_set_visible ((GtkWidget*) app->window.self, FALSE);
	}
	return TRUE;
}

// UgtkTraveler.download.view "key-press-event"
/*
static gboolean  on_traveler_key_press_event  (GtkWidget* widget, GdkEventKey* event, UgtkApp* app)
{
// ... old code ...
	switch (event->keyval) {
	case GDK_KEY_Delete:
		ugtk_app_delete_download (app, FALSE);
		return TRUE;

	case GDK_KEY_space:
		ugtk_app_switch_download_state (app);
		return TRUE;

	default:
		break;
	}

	return FALSE;
}
*/

static void ugtk_window_init_callback (struct UgtkWindow* window, UgtkApp* app)
{
	// UgtkTraveler - connect to selection model signals
	g_signal_connect (app->traveler.download.selection, "selection-changed",
			G_CALLBACK (on_download_selection_changed), app);
	g_signal_connect (app->traveler.download.selection, "selection-changed",
			G_CALLBACK (on_download_cursor_changed), app);
	g_signal_connect_after (app->traveler.category.selection, "selection-changed",
			G_CALLBACK (on_category_cursor_changed), app);
	g_signal_connect_after (app->traveler.state.selection, "selection-changed",
			G_CALLBACK (on_category_cursor_changed), app);

	// GTK4: Event handling changed - key-press-event and GdkEventKey removed
	// Using GtkShortcutController
	ugtk_add_shortcut (GTK_WIDGET(window->self), "Delete", on_shortcut_delete, app);
	ugtk_add_shortcut (GTK_WIDGET(window->self), "space", on_shortcut_space, app);
	ugtk_add_shortcut (GTK_WIDGET(window->self), "<Control>n", on_shortcut_new, app);
	ugtk_add_shortcut (GTK_WIDGET(window->self), "<Control>q", on_shortcut_quit, app);

	// Context menus and right-click handlers are set up in ugtk_context_menus_init()

	g_signal_connect (window->self, "close-request",
			G_CALLBACK (on_window_close_request), app);
	g_signal_connect_swapped (window->self, "destroy",
			G_CALLBACK (ugtk_app_quit), app);
}


// ----------------------------------------------------------------------------
// UgtkToolbar

static void ugtk_toolbar_init_callback (struct UgtkToolbar* toolbar, UgtkApp* app)
{
	// create new
	// g_signal_connect (toolbar->create, "clicked",
	// 		G_CALLBACK (on_create_download), app);
	g_signal_connect (toolbar->create_download, "clicked",
			G_CALLBACK (on_create_download), app);
	g_signal_connect_swapped (toolbar->create_category, "clicked",
			G_CALLBACK (ugtk_app_create_category), app);
	g_signal_connect_swapped (toolbar->create_sequence, "clicked",
			G_CALLBACK (ugtk_app_sequence_batch), app);
	g_signal_connect_swapped (toolbar->create_clipboard, "clicked",
			G_CALLBACK (ugtk_app_clipboard_batch), app);
	g_signal_connect_swapped (toolbar->create_torrent, "clicked",
			G_CALLBACK (ugtk_app_create_torrent), app);
	g_signal_connect_swapped (toolbar->create_metalink, "clicked",
			G_CALLBACK (ugtk_app_create_metalink), app);
	// save
	g_signal_connect_swapped (toolbar->save, "clicked",
			G_CALLBACK (ugtk_app_save), app);
	// change status
	g_signal_connect (toolbar->runnable, "clicked",
			G_CALLBACK (on_set_download_runnable), app);
	g_signal_connect_swapped (toolbar->pause, "clicked",
			G_CALLBACK (ugtk_app_pause_download), app);
	// change data
	g_signal_connect_swapped (toolbar->properties, "clicked",
			G_CALLBACK (ugtk_app_edit_download), app);
	// move
	g_signal_connect_swapped (toolbar->move_up, "clicked",
			G_CALLBACK (ugtk_app_move_download_up), app);
	g_signal_connect_swapped (toolbar->move_down, "clicked",
			G_CALLBACK (ugtk_app_move_download_down), app);
	g_signal_connect_swapped (toolbar->move_top, "clicked",
			G_CALLBACK (ugtk_app_move_download_top), app);
	g_signal_connect_swapped (toolbar->move_bottom, "clicked",
			G_CALLBACK (ugtk_app_move_download_bottom), app);
}

// ----------------------------------------------------------------------------
// functions for UgetNode.notification

static void node_inserted (UgetNode* node, UgetNode* sibling, UgetNode* child)
{
	UgtkApp*  app;
	int       pos;

	app = node->control->notifier->data;
	if (node == (UgetNode*) app->traveler.category.model->root) {
		// category inserted
		pos = uget_node_child_position (node, child) + 1; // +1 for "All Category"
		g_list_model_items_changed (
				G_LIST_MODEL (app->traveler.category.model), pos, 0, 1);
		// sync UgtkMenubar.download.move_to
		ugtk_menubar_sync_category (&app->menubar, app, TRUE);
	}
	else if (node == (UgetNode*) app->traveler.download.model->root) {
		// download inserted
		pos = uget_node_child_position (node, child);
		g_list_model_items_changed (
				G_LIST_MODEL (app->traveler.download.model), pos, 0, 1);
	}
}

static void node_removed (UgetNode* node, UgetNode* sibling, UgetNode* child)
{
	UgtkApp*  app;
	int       pos;

	app = node->control->notifier->data;
	if (node == (UgetNode*) app->traveler.category.model->root) {
		// category removed
		if (sibling)
			pos = uget_node_child_position (node, sibling);
		else
			pos = app->traveler.category.model->root->n_children;
		pos += 1;  // +1 for "All Category"
		g_list_model_items_changed (
				G_LIST_MODEL (app->traveler.category.model), pos, 1, 0);
		// sync UgtkMenubar.download.move_to
		ugtk_menubar_sync_category (&app->menubar, app, TRUE);
	}
	else if (node == (UgetNode*) app->traveler.download.model->root) {
		// download removed
		if (sibling)
			pos = uget_node_child_position (node, sibling);
		else
			pos = app->traveler.download.model->root->n_children;
		g_list_model_items_changed (
				G_LIST_MODEL (app->traveler.download.model), pos, 1, 0);
	}
}

static void node_updated (UgetNode* child)
{
	UgetNode*  node;
	UgtkApp*   app;
	int        pos;

	node = child->parent;
	if (node == NULL)
		return;
	app = node->control->notifier->data;
	if (node == (UgetNode*) app->traveler.category.model->root) {
		// category changed
		pos = uget_node_child_position (node, child) + 1;  // +1 for "All Category"
		g_list_model_items_changed (
				G_LIST_MODEL (app->traveler.category.model), pos, 1, 1);
		// sync UgtkMenubar.download.move_to
		ugtk_menubar_sync_category (&app->menubar, app, TRUE);
	}
	else if (node == (UgetNode*) app->traveler.download.model->root) {
		// download changed
		pos = uget_node_child_position (node, child);
		g_list_model_items_changed (
				G_LIST_MODEL (app->traveler.download.model), pos, 1, 1);
	}
}
