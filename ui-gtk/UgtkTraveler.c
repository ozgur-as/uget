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

#include <UgtkTraveler.h>
#include <UgtkApp.h>

// signal handler
static void on_state_cursor_changed (GtkTreeView* view, UgtkTraveler* traveler);
static void on_category_cursor_changed (GtkTreeView* view, UgtkTraveler* traveler);
static void on_download_cursor_changed (GtkTreeView* view, UgtkTraveler* traveler);
static void on_category_row_deleted (GtkTreeModel* model, GtkTreePath* path, UgtkTraveler* traveler);
static void on_category_row_deleted (GtkTreeModel* model, GtkTreePath* path, UgtkTraveler* traveler);
static void on_download_row_deleted (GtkTreeModel* model, GtkTreePath* path, UgtkTraveler* traveler);
static void on_download_view_clicked (GtkGestureClick* gesture, int n_press, double x, double y, gpointer user_data);
// static data
const static void*          sort_callbacks[UGTK_NODE_N_COLUMNS];
const static UgCompareFunc  compare_funcs[UGTK_NODE_N_COLUMNS];

void  ugtk_traveler_init (UgtkTraveler* traveler, UgtkApp* app)
{
	GtkScrolledWindow*  scroll;
	GtkTreePath*        path;
	GtkTreeViewColumn*  column;
	gint                nth;

	traveler->app = app;
	// status
	traveler->state.self = ugtk_node_view_new_for_state ();
	traveler->state.view = GTK_TREE_VIEW (traveler->state.self);
	traveler->state.model = ugtk_node_list_new (NULL, 4, TRUE);
	gtk_tree_view_set_model (traveler->state.view,
			GTK_TREE_MODEL (traveler->state.model));

	// category
	traveler->category.self = gtk_scrolled_window_new ();
	traveler->category.view = (GtkTreeView*) ugtk_node_view_new_for_category ();
	traveler->category.model = ugtk_node_tree_new (&app->sorted, TRUE);
	ugtk_node_tree_set_prefix (traveler->category.model, &app->mix, 1);
	gtk_tree_view_set_model (traveler->category.view,
			GTK_TREE_MODEL (traveler->category.model));
	gtk_widget_set_size_request (traveler->category.self, 165, 100);
	scroll = GTK_SCROLLED_WINDOW (traveler->category.self);
	// gtk_scrolled_window_set_shadow_type (scroll, GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (scroll,
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_child (scroll,
			GTK_WIDGET (traveler->category.view));

	// download
	traveler->download.self = gtk_scrolled_window_new ();
	traveler->download.view = (GtkTreeView*) ugtk_node_view_new_for_download ();
	traveler->download.model = ugtk_node_tree_new (NULL, TRUE);
	gtk_tree_view_set_model (traveler->download.view,
			GTK_TREE_MODEL (traveler->download.model));
	scroll = GTK_SCROLLED_WINDOW (traveler->download.self);
	// gtk_scrolled_window_set_shadow_type (scroll, GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (scroll,
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_child (scroll,
			GTK_WIDGET (traveler->download.view));

	// Click to unselect (UX improvement)
	GtkGesture *gesture = gtk_gesture_click_new ();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), GDK_BUTTON_PRIMARY);
	g_signal_connect (gesture, "pressed", G_CALLBACK (on_download_view_clicked), traveler->download.view);
	gtk_widget_add_controller (GTK_WIDGET (traveler->download.view), GTK_EVENT_CONTROLLER (gesture));

	path = gtk_tree_path_new_first ();
	gtk_tree_view_set_cursor (traveler->state.view, path, NULL, FALSE);
	gtk_tree_view_set_cursor (traveler->category.view, path, NULL, FALSE);
	gtk_tree_path_free (path);

	// cursor position
	traveler->state.cursor.pos = -1;
	traveler->category.cursor.pos = -1;
	traveler->download.cursor.pos = -1;
	// signal
	g_signal_connect (traveler->state.view, "cursor-changed",
			G_CALLBACK (on_state_cursor_changed), traveler);
	g_signal_connect (traveler->category.view, "cursor-changed",
			G_CALLBACK (on_category_cursor_changed), traveler);
	g_signal_connect (traveler->download.view, "cursor-changed",
			G_CALLBACK (on_download_cursor_changed), traveler);
	g_signal_connect (traveler->category.model, "row-deleted",
			G_CALLBACK (on_category_row_deleted), traveler);
	g_signal_connect (traveler->download.model, "row-deleted",
			G_CALLBACK (on_download_row_deleted), traveler);

	for (nth = UGTK_NODE_COLUMN_STATE;  nth < UGTK_NODE_N_COLUMNS;  nth++) {
		column = gtk_tree_view_get_column (traveler->download.view, nth);
		g_signal_connect (column, "clicked",
				G_CALLBACK (sort_callbacks[nth]), traveler);
	}
}

void  ugtk_traveler_select_category (UgtkTraveler* traveler,
                                     int nth_category, int nth_state)
{
	GtkTreePath*  path;

	if (nth_category != -1) {
		path = gtk_tree_path_new ();
		gtk_tree_path_append_index (path, nth_category);
		gtk_tree_view_set_cursor (traveler->category.view, path, NULL, FALSE);
		gtk_tree_path_free (path);
	}

	if (nth_state != -1) {
		path = gtk_tree_path_new ();
		gtk_tree_path_append_index (path, nth_state);
		gtk_tree_view_set_cursor (traveler->state.view, path, NULL, FALSE);
		gtk_tree_path_free (path);
	}
}

void  ugtk_traveler_set_cursor (UgtkTraveler* traveler, UgetNode* node)
{
	GtkTreePath* path;
	GtkTreeIter  iter;

	if (node && ugtk_traveler_get_iter (traveler, &iter, node)) {
		path = gtk_tree_model_get_path (
				GTK_TREE_MODEL (traveler->download.model), &iter);
		if (path) {
//			traveler->download.cursor.node = iter.user_data;
//			traveler->download.cursor.pos  = *gtk_tree_path_get_indices (path);
			gtk_tree_view_set_cursor (traveler->download.view,
			                          path, NULL, FALSE);
			gtk_tree_path_free (path);
		}
	}
}

UgetNode* ugtk_traveler_get_cursor (UgtkTraveler* traveler)
{
	GtkTreePath* path;
	int          nth;

	gtk_tree_view_get_cursor (traveler->download.view, &path, NULL);
	if (path == NULL)
		return NULL;
	nth = *gtk_tree_path_get_indices (path);
	gtk_tree_path_free (path);
	return uget_node_nth_child (traveler->download.model->root, nth);
}

gboolean  ugtk_traveler_get_iter (UgtkTraveler* traveler,
                                  GtkTreeIter* iter, UgetNode* node)
{
	if (node->parent == (UgetNode*) traveler->download.model->root) {
		iter->stamp = traveler->download.model->stamp;
		iter->user_data = node;
		return TRUE;
	}

	for (node = node->fake;  node;  node = node->peer) {
		if (ugtk_traveler_get_iter (traveler, iter, node))
			return TRUE;
	}
	return FALSE;
}

GList* ugtk_traveler_get_selected (UgtkTraveler* traveler)
{
	GtkTreeSelection* selection;
	GtkTreeModel*     model;
	GtkTreeIter       iter;
	GList*      list;
	GList*      nodes;
	GList*      cur;

	selection = gtk_tree_view_get_selection (traveler->download.view);
	list = gtk_tree_selection_get_selected_rows (selection, &model);
	nodes = NULL;
	for (cur = list;  cur;  cur = cur->next) {
		gtk_tree_model_get_iter (model, &iter, cur->data);
		nodes = g_list_prepend (nodes, iter.user_data);
	}

	g_list_free_full (list, (GDestroyNotify) gtk_tree_path_free);
	return nodes;
}

void  ugtk_traveler_set_selected (UgtkTraveler* traveler,
                                  GList*        nodes)
{
	GtkTreeSelection* selection;
	GtkTreeIter       iter;

	selection = gtk_tree_view_get_selection (traveler->download.view);
	gtk_tree_selection_unselect_all (selection);
	for (;  nodes;  nodes = nodes->next) {
		if (nodes->data == NULL)
			continue;
		if (ugtk_traveler_get_iter (traveler, &iter, nodes->data))
			gtk_tree_selection_select_iter (selection, &iter);
	}
}

GList*  ugtk_traveler_reserve_selection (UgtkTraveler* traveler)
{
	GList*    list;
	GList*    link;

	list = ugtk_traveler_get_selected (traveler);
	for (link = list;  link;  link = link->next)
		link->data = ((UgetNode*)link->data)->base;

	traveler->reserved.list = list;
	traveler->reserved.node = traveler->download.cursor.node;
	if (traveler->reserved.node)
		traveler->reserved.node = traveler->reserved.node->base;
	return list;
}

void  ugtk_traveler_restore_selection (UgtkTraveler* traveler)
{
	GList*    list;
	UgetNode* node;

	list = traveler->reserved.list;
	node = traveler->reserved.node;
	ugtk_traveler_set_cursor (traveler, node);
	ugtk_traveler_set_selected (traveler, list);
	traveler->reserved.list = NULL;
	traveler->reserved.node = NULL;
}

gint  ugtk_traveler_move_selected_up (UgtkTraveler* traveler)
{
	GtkTreePath*  path;
	UgetNode* node;
	UgetNode* prev;
	UgetNode* top;
	GList*    list;
	GList*    link;
	int       counts = 0;

	// Automatically switch to manual sort if needed
	UgtkApp* app = (UgtkApp*) traveler->app;
	if (app->setting.download_column.sort.nth != UGTK_NODE_COLUMN_STATE) {
		ugtk_traveler_set_sorting(traveler, TRUE, UGTK_NODE_COLUMN_STATE, GTK_SORT_ASCENDING);
		app->setting.download_column.sort.nth = UGTK_NODE_COLUMN_STATE;
	}

	list = ugtk_traveler_get_selected (traveler);
	list = g_list_reverse (list);
	for (top = NULL, link = list;  link;  link = link->next) {
		node = link->data;
		prev = node->prev;
		if (top == prev) {
			top  = node;
			continue;
		}
		top = node;

		for (;;) {
			if (node->real == NULL || prev->real == NULL)
				break;
			if (node->real->parent != prev->real->parent)
				break;
			node = node->real;
			prev = prev->real;
		}
		uget_node_move (node->parent, prev, node);
		counts++;
	}

	if (counts > 0) {
		// scroll to first selected download and move cursor
		node = list->data;
		path = gtk_tree_path_new_from_indices (
				uget_node_child_position (node->parent, node), -1);
		gtk_tree_view_scroll_to_cell (traveler->download.view,
	                                  path, NULL, FALSE, 0, 0);
		gtk_tree_view_set_cursor (traveler->download.view, path, NULL, FALSE);
		gtk_tree_path_free (path);
		// redraw
		gtk_widget_queue_draw ((GtkWidget*) traveler->download.view);
		// change selected indices
		ugtk_traveler_set_selected (traveler, list);
	}
	g_list_free (list);
	return counts;
}

gint  ugtk_traveler_move_selected_down (UgtkTraveler* traveler)
{
	GtkTreePath*  path;
	UgetNode* node;
	UgetNode* next;
	UgetNode* bottom;
	GList*    list;
	GList*    link;
	int       counts = 0;

	// Automatically switch to manual sort if needed
	UgtkApp* app = (UgtkApp*) traveler->app;
	if (app->setting.download_column.sort.nth != UGTK_NODE_COLUMN_STATE) {
		ugtk_traveler_set_sorting(traveler, TRUE, UGTK_NODE_COLUMN_STATE, GTK_SORT_ASCENDING);
		app->setting.download_column.sort.nth = UGTK_NODE_COLUMN_STATE;
	}

	list = ugtk_traveler_get_selected (traveler);
	for (bottom = NULL, link = list;  link;  link = link->next) {
		node = link->data;
		next = node->next;
		if (bottom == next) {
			bottom  = node;
			continue;
		}
		bottom = node;

		for (;;) {
			if (node->real == NULL || next->real == NULL)
				break;
			if (node->real->parent != next->real->parent)
				break;
			node = node->real;
			next = next->real;
		}
		uget_node_move (node->parent, next->next, node);
		counts++;
	}

	if (counts > 0) {
		// scroll to last selected download and move cursor
		node = list->data;
		path = gtk_tree_path_new_from_indices (
				uget_node_child_position (node->parent, node), -1);
		gtk_tree_view_scroll_to_cell (traveler->download.view,
	                                  path, NULL, FALSE, 0, 0);
		gtk_tree_view_set_cursor (traveler->download.view, path, NULL, FALSE);
		gtk_tree_path_free (path);
		// redraw
		gtk_widget_queue_draw ((GtkWidget*) traveler->download.view);
		// change selected indices
		ugtk_traveler_set_selected (traveler, list);
	}
	g_list_free (list);
	return counts;
}

gint  ugtk_traveler_move_selected_top (UgtkTraveler* traveler)
{
	GtkTreePath* path;
	UgetNode* node;
	UgetNode* sibling;
	UgetNode* top;
	GList*    list;
	GList*    link;
	int       counts = 0;

	// Automatically switch to manual sort if needed
	UgtkApp* app = (UgtkApp*) traveler->app;
	if (app->setting.download_column.sort.nth != UGTK_NODE_COLUMN_STATE) {
		ugtk_traveler_set_sorting(traveler, TRUE, UGTK_NODE_COLUMN_STATE, GTK_SORT_ASCENDING);
		app->setting.download_column.sort.nth = UGTK_NODE_COLUMN_STATE;
	}

	list = ugtk_traveler_get_selected (traveler);
	list = g_list_reverse (list);
	node = list->data;
	top  = node->parent->children;
	for (link = list;  link;  link = link->next) {
		node = link->data;
		if (top == node) {
			top  = top->next;
			continue;
		}

		sibling = top;
		for (;;) {
			if (node->real == NULL || sibling->real == NULL)
				break;
			if (node->real->parent != sibling->real->parent)
				break;
			node    = node->real;
			sibling = sibling->real;
		}
		uget_node_move (node->parent, sibling, node);
		counts++;
	}

	if (counts > 0) {
		// scroll to top and move cursor
		gtk_tree_view_scroll_to_point (traveler->download.view, -1, 0);
		path = gtk_tree_path_new_first ();
		gtk_tree_view_set_cursor (traveler->download.view, path, NULL, FALSE);
		gtk_tree_path_free (path);
		// redraw
		gtk_widget_queue_draw ((GtkWidget*) traveler->download.view);
		// change selected indices
		ugtk_traveler_set_selected (traveler, list);
	}
	g_list_free (list);
	return counts;
}

gint  ugtk_traveler_move_selected_bottom (UgtkTraveler* traveler)
{
	GtkTreePath*  path;
	UgetNode* node;
	UgetNode* sibling;
	UgetNode* bottom;
	GList*    list;
	GList*    link;
	int       counts = 0;

	// Automatically switch to manual sort if needed
	UgtkApp* app = (UgtkApp*) traveler->app;
	if (app->setting.download_column.sort.nth != UGTK_NODE_COLUMN_STATE) {
		ugtk_traveler_set_sorting(traveler, TRUE, UGTK_NODE_COLUMN_STATE, GTK_SORT_ASCENDING);
		app->setting.download_column.sort.nth = UGTK_NODE_COLUMN_STATE;
	}

	list = ugtk_traveler_get_selected (traveler);
	node = list->data;
	bottom = node->parent->last;
	for (link = list;  link;  link = link->next) {
		node = link->data;
		if (bottom == node) {
			bottom  = bottom->prev;
			continue;
		}

		sibling = bottom;
		for (;;) {
			if (node->real == NULL || sibling->real == NULL)
				break;
			if (node->real->parent != sibling->real->parent)
				break;
			node    = node->real;
			sibling = sibling->real;
		}
		// check this for move to bottom only
		if (sibling->next == node)
			continue;
		// node will be inserted after sibling
		uget_node_move (node->parent, sibling->next, node);
		counts++;
	}

	if (counts > 0) {
		// scroll to bottom and move cursor
		node = list->data;
		path = gtk_tree_path_new_from_indices (
				node->parent->n_children -1, -1);
		gtk_tree_view_scroll_to_cell (traveler->download.view,
	                                  path, NULL, FALSE, 0, 0);
		gtk_tree_view_set_cursor (traveler->download.view, path, NULL, FALSE);
		gtk_tree_path_free (path);
		// redraw
		gtk_widget_queue_draw ((GtkWidget*) traveler->download.view);
		// change selected indices
		ugtk_traveler_set_selected (traveler, list);
	}
	g_list_free (list);
	return counts;
}

void  ugtk_traveler_set_sorting (UgtkTraveler*  traveler,
                                 gboolean       sortable,
                                 UgtkNodeColumn nth_col,
                                 GtkSortType    type)
{
	GtkTreeViewColumn*  column;
	UgtkNodeColumn      pos;
	GList*  selected;

	for (pos = 0;  pos < UGTK_NODE_N_COLUMNS;  pos++) {
		column = gtk_tree_view_get_column (traveler->download.view, pos);
		gtk_tree_view_column_set_clickable (column, sortable);
		// clear column sort status
		gtk_tree_view_column_set_sort_order (column, GTK_SORT_ASCENDING);
		gtk_tree_view_column_set_sort_indicator (column, FALSE);
	}

	if (nth_col >= UGTK_NODE_N_COLUMNS)
		return;

	selected = ugtk_traveler_get_selected (traveler);
	if (nth_col <= 0)
		uget_app_set_sorting ((UgetApp*) traveler->app, NULL, FALSE);
	else {
		column = gtk_tree_view_get_column (traveler->download.view, nth_col);
		gtk_tree_view_column_set_sort_order (column, type);
		gtk_tree_view_column_set_sort_indicator (column, TRUE);
		uget_app_set_sorting ((UgetApp*) traveler->app, compare_funcs[nth_col],
				(type == GTK_SORT_DESCENDING) ? TRUE : FALSE);
	}
	ugtk_traveler_set_selected (traveler, selected);
	g_list_free (selected);
	// redraw
	gtk_widget_queue_draw ((GtkWidget*) traveler->download.view);
}

// ----------------------------------------------------------------------------
// signal handler
static void on_state_cursor_changed (GtkTreeView* view, UgtkTraveler* traveler)
{
	GtkTreeModel*  model;
	GtkTreePath*   path;
	GtkTreeIter    iter;

	// clear download cursor
	traveler->download.cursor.node = NULL;

	traveler->state.cursor.pos_last = traveler->state.cursor.pos;
	gtk_tree_view_get_cursor (view, &path, NULL);
	if (path == NULL) {
		traveler->state.cursor.pos  = -1;
		traveler->state.cursor.node = NULL;
		return;
	}
	if (traveler->state.cursor.pos == gtk_tree_path_get_indices (path)[0])
		return;

	traveler->state.cursor.pos = gtk_tree_path_get_indices (path)[0];
	model = gtk_tree_view_get_model (view);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);
	traveler->state.cursor.node = iter.user_data;

	// change download.model and refresh it's view
	gtk_tree_view_set_model (traveler->download.view, NULL);
	if (iter.user_data) {
		traveler->download.model->root = iter.user_data;
		gtk_tree_view_set_model (traveler->download.view,
		                         GTK_TREE_MODEL (traveler->download.model));
	}
}

static void on_category_cursor_changed (GtkTreeView* view, UgtkTraveler* traveler)
{
	GtkTreeModel*  model;
	GtkTreePath*   path;
	GtkTreeIter    iter;

	// clear download cursor
	traveler->download.cursor.node = NULL;

	traveler->category.cursor.pos_last = traveler->category.cursor.pos;
	gtk_tree_view_get_cursor (view, &path, NULL);
	if (path == NULL) {
		traveler->category.cursor.pos  = -1;
		traveler->category.cursor.node = NULL;
		return;
	}
	if (traveler->category.cursor.pos == gtk_tree_path_get_indices (path)[0])
		return;

	traveler->category.cursor.pos = gtk_tree_path_get_indices (path)[0];
	model = gtk_tree_view_get_model (view);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);
	traveler->category.cursor.node = iter.user_data;

	// change state.model and refresh it's view
	model = GTK_TREE_MODEL (traveler->state.model);
	path = gtk_tree_path_new_from_indices (traveler->state.cursor.pos, -1);
	gtk_tree_view_set_model (traveler->state.view, NULL);
	traveler->state.model->root = iter.user_data;
	gtk_tree_view_set_model (traveler->state.view, model);
	gtk_tree_view_set_cursor (traveler->state.view, path, NULL, FALSE);
	gtk_tree_path_free (path);
	// traveler->state.view will emit signal "cursor_changed" and
	// call on_state_cursor_changed()
}

static void on_download_cursor_changed (GtkTreeView* view, UgtkTraveler* traveler)
{
	GtkTreeModel*  model;
	GtkTreePath*   path;
	GtkTreeIter    iter;

	traveler->download.cursor.pos_last = traveler->download.cursor.pos;
	gtk_tree_view_get_cursor (view, &path, NULL);
	if (path == NULL) {
		traveler->download.cursor.pos  = -1;
		traveler->download.cursor.node = NULL;
		return;
	}

	traveler->download.cursor.pos = gtk_tree_path_get_indices (path)[0];
	model = gtk_tree_view_get_model (view);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);
	traveler->download.cursor.node = iter.user_data;
}

static void on_category_row_deleted (GtkTreeModel* model, GtkTreePath* p, UgtkTraveler* traveler)
{
	GtkTreePath* path;
	GtkTreeIter  iter;

	// update cursor position when row deleted
	gtk_tree_view_get_cursor (traveler->category.view, &path, NULL);
	if (path == NULL) {
		traveler->category.cursor.pos  = 0;
		traveler->category.cursor.node = NULL;
		return;
	}
	gtk_tree_model_get_iter (model, &iter, path);
	traveler->category.cursor.node = iter.user_data;
	traveler->category.cursor.pos  = gtk_tree_path_get_indices (path)[0];
	gtk_tree_path_free (path);
}

static void on_download_row_deleted (GtkTreeModel* model, GtkTreePath* p, UgtkTraveler* traveler)
{
	GtkTreePath* path;
	GtkTreeIter  iter;

	// update cursor position when row deleted
	gtk_tree_view_get_cursor (traveler->download.view, &path, NULL);
	if (path == NULL) {
		traveler->download.cursor.node = NULL;
		traveler->download.cursor.pos  = 0;
		return;
	}
	gtk_tree_model_get_iter (model, &iter, path);
	traveler->download.cursor.node = iter.user_data;
	traveler->download.cursor.pos  = gtk_tree_path_get_indices (path)[0];
	gtk_tree_path_free (path);
}

static void on_download_view_clicked (GtkGestureClick* gesture, int n_press, double x, double y, gpointer user_data)
{
	GtkTreeView *view = GTK_TREE_VIEW (user_data);
	GtkTreeSelection *selection;
	GtkTreePath *path = NULL;
	int bx, by;

	// Convert widget coordinates to bin window coordinates
	gtk_tree_view_convert_widget_to_bin_window_coords (view, (int)x, (int)y, &bx, &by);

	// Check if we clicked on a row
	if (!gtk_tree_view_get_path_at_pos (view, bx, by, &path, NULL, NULL, NULL)) {
		// No path means empty space -> unselect all
		selection = gtk_tree_view_get_selection (view);
		gtk_tree_selection_unselect_all (selection);
	} else {
		// Valid path, let default handler handle it (or just free path)
		gtk_tree_path_free (path);
	}
}

// ----------------------------------------------------------------------------
// signal handler for GtkTreeViewColumn

static void ugtk_tree_view_column_clicked (GtkTreeViewColumn* column,
                                           UgtkNodeColumn     nth_column,
                                           UgtkTraveler*      traveler)
{
	GtkSortType  sorttype;
	UgtkApp*     app;
	gint         pos;

	// get column sort status
	column = gtk_tree_view_get_column (traveler->download.view, nth_column);
	sorttype = gtk_tree_view_column_get_sort_order (column);
	if (gtk_tree_view_column_get_sort_indicator (column)) {
		if (sorttype == GTK_SORT_ASCENDING)
			sorttype  = GTK_SORT_DESCENDING;
		else
			sorttype  = GTK_SORT_ASCENDING;
	}

	// clear column sort status
	for (pos = UGTK_NODE_COLUMN_NAME;  pos < UGTK_NODE_N_COLUMNS;  pos++) {
		GtkTreeViewColumn*  tmp_column;

		tmp_column = gtk_tree_view_get_column (traveler->download.view, pos);
		gtk_tree_view_column_set_sort_order (tmp_column, GTK_SORT_ASCENDING);
		gtk_tree_view_column_set_sort_indicator (tmp_column, FALSE);
	}

	app = traveler->app;
	app->setting.download_column.sort.nth = nth_column;
	app->setting.download_column.sort.type = sorttype;
	if (nth_column > UGTK_NODE_COLUMN_STATE) {
		gtk_tree_view_column_set_sort_order (column, sorttype);
		gtk_tree_view_column_set_sort_indicator (column, TRUE);
	}
	uget_app_set_sorting ((UgetApp*) app, compare_funcs[nth_column],
			(sorttype == GTK_SORT_DESCENDING) ? TRUE : FALSE);
	// Any Category/Status can't move download position if they were sorted.
	ugtk_app_decide_download_sensitive (app);
	gtk_widget_queue_draw ((GtkWidget*) traveler->download.view);
}

static void on_state_column_clicked (GtkTreeViewColumn *column, UgtkTraveler* traveler)
{
	ugtk_tree_view_column_clicked (column, UGTK_NODE_COLUMN_STATE, traveler);
}

static void on_name_column_clicked (GtkTreeViewColumn *column, UgtkTraveler* traveler)
{
	ugtk_tree_view_column_clicked (column, UGTK_NODE_COLUMN_NAME, traveler);
}

static void on_complete_column_clicked (GtkTreeViewColumn *column, UgtkTraveler* traveler)
{
	ugtk_tree_view_column_clicked (column, UGTK_NODE_COLUMN_COMPLETE, traveler);
}

static void on_size_column_clicked (GtkTreeViewColumn *column, UgtkTraveler* traveler)
{
	ugtk_tree_view_column_clicked (column, UGTK_NODE_COLUMN_TOTAL, traveler);
}

static void on_percent_column_clicked (GtkTreeViewColumn *column, UgtkTraveler* traveler)
{
	ugtk_tree_view_column_clicked (column, UGTK_NODE_COLUMN_PERCENT, traveler);
}

static void on_elapsed_column_clicked (GtkTreeViewColumn *column, UgtkTraveler* traveler)
{
	ugtk_tree_view_column_clicked (column, UGTK_NODE_COLUMN_ELAPSED, traveler);
}

static void on_left_column_clicked (GtkTreeViewColumn *column, UgtkTraveler* traveler)
{
	ugtk_tree_view_column_clicked (column, UGTK_NODE_COLUMN_LEFT, traveler);
}

static void on_speed_column_clicked (GtkTreeViewColumn *column, UgtkTraveler* traveler)
{
	ugtk_tree_view_column_clicked (column, UGTK_NODE_COLUMN_SPEED, traveler);
}

static void on_upload_speed_column_clicked (GtkTreeViewColumn *column, UgtkTraveler* traveler)
{
	ugtk_tree_view_column_clicked (column, UGTK_NODE_COLUMN_UPLOAD_SPEED, traveler);
}

static void on_uploaded_column_clicked (GtkTreeViewColumn *column, UgtkTraveler* traveler)
{
	ugtk_tree_view_column_clicked (column, UGTK_NODE_COLUMN_UPLOADED, traveler);
}

static void on_ratio_column_clicked (GtkTreeViewColumn *column, UgtkTraveler* traveler)
{
	ugtk_tree_view_column_clicked (column, UGTK_NODE_COLUMN_RATIO, traveler);
}

static void on_retry_column_clicked (GtkTreeViewColumn *column, UgtkTraveler* traveler)
{
	ugtk_tree_view_column_clicked (column, UGTK_NODE_COLUMN_RETRY, traveler);
}

static void on_category_column_clicked (GtkTreeViewColumn *column, UgtkTraveler* traveler)
{
	ugtk_tree_view_column_clicked (column, UGTK_NODE_COLUMN_CATEGORY, traveler);
}

static void on_url_column_clicked (GtkTreeViewColumn *column, UgtkTraveler* traveler)
{
	ugtk_tree_view_column_clicked (column, UGTK_NODE_COLUMN_URI, traveler);
}

static void on_added_on_column_clicked (GtkTreeViewColumn *column, UgtkTraveler* traveler)
{
	ugtk_tree_view_column_clicked (column, UGTK_NODE_COLUMN_ADDED_ON, traveler);
}

static void on_completed_on_column_clicked (GtkTreeViewColumn *column, UgtkTraveler* traveler)
{
	ugtk_tree_view_column_clicked (column, UGTK_NODE_COLUMN_COMPLETED_ON, traveler);
}

// ----------------------------------------------------------------------------
// static data

const static void* sort_callbacks[UGTK_NODE_N_COLUMNS] =
{
	on_state_column_clicked,
	on_name_column_clicked,
	on_complete_column_clicked,
	on_size_column_clicked,
	on_percent_column_clicked,
	on_elapsed_column_clicked,
	on_left_column_clicked,
	on_speed_column_clicked,
	on_upload_speed_column_clicked,
	on_uploaded_column_clicked,
	on_ratio_column_clicked,
	on_retry_column_clicked,
	on_category_column_clicked,
	on_url_column_clicked,
	on_added_on_column_clicked,
	on_completed_on_column_clicked,
};

const static UgCompareFunc  compare_funcs[UGTK_NODE_N_COLUMNS] =
{
	(UgCompareFunc) NULL,
	(UgCompareFunc) uget_node_compare_name,
	(UgCompareFunc) uget_node_compare_complete,
	(UgCompareFunc) uget_node_compare_size,
	(UgCompareFunc) uget_node_compare_percent,
	(UgCompareFunc) uget_node_compare_elapsed,
	(UgCompareFunc) uget_node_compare_left,
	(UgCompareFunc) uget_node_compare_speed,
	(UgCompareFunc) uget_node_compare_upload_speed,
	(UgCompareFunc) uget_node_compare_uploaded,
	(UgCompareFunc) uget_node_compare_ratio,
	(UgCompareFunc) uget_node_compare_retry,
	(UgCompareFunc) uget_node_compare_parent_name,
	(UgCompareFunc) uget_node_compare_uri,
	(UgCompareFunc) uget_node_compare_added_time,
	(UgCompareFunc) uget_node_compare_completed_time,
};

