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
#include <UgtkNodeObject.h>
#include <UgtkApp.h>

// signal handlers
static void on_state_selection_changed (GtkSelectionModel* model, guint pos, guint n_items, UgtkTraveler* traveler);
static void on_category_selection_changed (GtkSelectionModel* model, guint pos, guint n_items, UgtkTraveler* traveler);
static void on_download_selection_changed (GtkSelectionModel* model, guint pos, guint n_items, UgtkTraveler* traveler);

// static data
const static UgCompareFunc  compare_funcs[UGTK_NODE_N_COLUMNS];

// helper: get UgetNode* at position in a GListModel
static UgetNode* get_node_at (GListModel* model, guint pos)
{
	UgtkNodeObject* obj;
	UgetNode* node;

	obj = g_list_model_get_item (model, pos);
	if (obj == NULL)
		return NULL;
	node = obj->node;
	g_object_unref (obj);
	return node;
}

// helper: find position of a UgetNode in a GListModel
static int find_node_position (GListModel* model, UgetNode* target)
{
	guint n = g_list_model_get_n_items (model);
	guint i;

	for (i = 0; i < n; i++) {
		UgtkNodeObject* obj = g_list_model_get_item (model, i);
		if (obj) {
			UgetNode* node = obj->node;
			g_object_unref (obj);
			if (node == target)
				return (int) i;
		}
	}
	return -1;
}

void  ugtk_traveler_init (UgtkTraveler* traveler, UgtkApp* app)
{
	GtkScrolledWindow*  scroll;

	traveler->app = app;

	// --- state ---
	traveler->state.model = ugtk_node_list_new (NULL, 4, TRUE);
	traveler->state.selection = gtk_single_selection_new (
			G_LIST_MODEL (g_object_ref (traveler->state.model)));
	gtk_single_selection_set_autoselect (traveler->state.selection, FALSE);
	gtk_single_selection_set_can_unselect (traveler->state.selection, TRUE);

	traveler->state.self = ugtk_node_view_new_for_state ();
	traveler->state.view = GTK_LIST_VIEW (traveler->state.self);
	gtk_list_view_set_model (traveler->state.view,
			GTK_SELECTION_MODEL (traveler->state.selection));

	// --- category ---
	traveler->category.model = ugtk_node_tree_new (&app->sorted, TRUE);
	ugtk_node_tree_set_prefix (traveler->category.model, &app->mix, 1);
	traveler->category.selection = gtk_single_selection_new (
			G_LIST_MODEL (g_object_ref (traveler->category.model)));
	gtk_single_selection_set_autoselect (traveler->category.selection, FALSE);
	gtk_single_selection_set_can_unselect (traveler->category.selection, TRUE);

	traveler->category.self = gtk_scrolled_window_new ();
	GtkWidget* cat_view_widget = ugtk_node_view_new_for_category ();
	traveler->category.view = GTK_LIST_VIEW (cat_view_widget);
	gtk_list_view_set_model (traveler->category.view,
			GTK_SELECTION_MODEL (traveler->category.selection));
	gtk_widget_set_size_request (traveler->category.self, 165, 100);
	scroll = GTK_SCROLLED_WINDOW (traveler->category.self);
	gtk_scrolled_window_set_policy (scroll,
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_child (scroll, GTK_WIDGET (traveler->category.view));

	// --- download ---
	traveler->download.model = ugtk_node_tree_new (NULL, TRUE);
	traveler->download.selection = gtk_multi_selection_new (
			G_LIST_MODEL (g_object_ref (traveler->download.model)));

	traveler->download.self = gtk_scrolled_window_new ();
	GtkWidget* dl_view_widget = ugtk_node_view_new_for_download ();
	traveler->download.view = GTK_COLUMN_VIEW (dl_view_widget);
	gtk_column_view_set_model (traveler->download.view,
			GTK_SELECTION_MODEL (traveler->download.selection));
	scroll = GTK_SCROLLED_WINDOW (traveler->download.self);
	gtk_scrolled_window_set_policy (scroll,
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_child (scroll, GTK_WIDGET (traveler->download.view));

	// cursor position (must be set before signals fire)
	traveler->state.cursor.pos = -1;
	traveler->category.cursor.pos = -1;
	traveler->download.cursor.pos = -1;

	// signals (must be connected before initial selection)
	g_signal_connect (traveler->state.selection, "selection-changed",
			G_CALLBACK (on_state_selection_changed), traveler);
	g_signal_connect (traveler->category.selection, "selection-changed",
			G_CALLBACK (on_category_selection_changed), traveler);
	g_signal_connect (traveler->download.selection, "selection-changed",
			G_CALLBACK (on_download_selection_changed), traveler);

	// GtkSingleSelection autoselects position 0 during construction
	// (default autoselect=TRUE), so set_selected(0) is a no-op.
	// Manually initialize the state model with the first category node.
	{
		UgetNode* cnode = get_node_at (
				G_LIST_MODEL (traveler->category.model), 0);
		if (cnode) {
			traveler->category.cursor.pos = 0;
			traveler->category.cursor.node = cnode;
			traveler->state.model->root = cnode;
			ugtk_node_list_refresh (traveler->state.model);
		}
	}
	// State model now has items; select first state to populate downloads
	gtk_single_selection_set_selected (traveler->state.selection, 0);
}

void  ugtk_traveler_select_category (UgtkTraveler* traveler,
                                     int nth_category, int nth_state)
{
	if (nth_category >= 0)
		gtk_single_selection_set_selected (traveler->category.selection, nth_category);
	if (nth_state >= 0)
		gtk_single_selection_set_selected (traveler->state.selection, nth_state);
}

void  ugtk_traveler_set_cursor (UgtkTraveler* traveler, UgetNode* node)
{
	int pos;

	if (node == NULL)
		return;

	pos = find_node_position (G_LIST_MODEL (traveler->download.model), node);
	if (pos >= 0) {
		// Select just this item
		GtkBitset* bitset = gtk_bitset_new_empty ();
		gtk_bitset_add (bitset, pos);
		gtk_selection_model_set_selection (
				GTK_SELECTION_MODEL (traveler->download.selection),
				bitset, bitset);
		gtk_bitset_unref (bitset);
	}
}

UgetNode* ugtk_traveler_get_cursor (UgtkTraveler* traveler)
{
	return traveler->download.cursor.node;
}

GList* ugtk_traveler_get_selected (UgtkTraveler* traveler)
{
	GtkBitset*    bitset;
	GtkBitsetIter iter;
	guint         pos;
	GList*        nodes = NULL;

	bitset = gtk_selection_model_get_selection (
			GTK_SELECTION_MODEL (traveler->download.selection));

	if (gtk_bitset_iter_init_first (&iter, bitset, &pos)) {
		do {
			UgetNode* node = get_node_at (
					G_LIST_MODEL (traveler->download.model), pos);
			if (node)
				nodes = g_list_prepend (nodes, node);
		} while (gtk_bitset_iter_next (&iter, &pos));
	}
	gtk_bitset_unref (bitset);
	return nodes;
}

void  ugtk_traveler_set_selected (UgtkTraveler* traveler, GList* nodes)
{
	GtkBitset* select_bitset;
	GtkBitset* mask_bitset;
	guint n_items;

	n_items = g_list_model_get_n_items (G_LIST_MODEL (traveler->download.model));
	select_bitset = gtk_bitset_new_empty ();
	mask_bitset = gtk_bitset_new_range (0, n_items);

	for (; nodes; nodes = nodes->next) {
		if (nodes->data == NULL)
			continue;
		int pos = find_node_position (
				G_LIST_MODEL (traveler->download.model), nodes->data);
		if (pos >= 0)
			gtk_bitset_add (select_bitset, pos);
	}

	gtk_selection_model_set_selection (
			GTK_SELECTION_MODEL (traveler->download.selection),
			select_bitset, mask_bitset);
	gtk_bitset_unref (select_bitset);
	gtk_bitset_unref (mask_bitset);
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
	UgetNode* node;
	UgetNode* prev;
	UgetNode* top;
	GList*    list;
	GList*    link;
	int       counts = 0;

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
		ugtk_node_tree_refresh (traveler->download.model);
		ugtk_traveler_set_selected (traveler, list);
	}
	g_list_free (list);
	return counts;
}

gint  ugtk_traveler_move_selected_down (UgtkTraveler* traveler)
{
	UgetNode* node;
	UgetNode* next;
	UgetNode* bottom;
	GList*    list;
	GList*    link;
	int       counts = 0;

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
		ugtk_node_tree_refresh (traveler->download.model);
		ugtk_traveler_set_selected (traveler, list);
	}
	g_list_free (list);
	return counts;
}

gint  ugtk_traveler_move_selected_top (UgtkTraveler* traveler)
{
	UgetNode* node;
	UgetNode* sibling;
	UgetNode* top;
	GList*    list;
	GList*    link;
	int       counts = 0;

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
		ugtk_node_tree_refresh (traveler->download.model);
		ugtk_traveler_set_selected (traveler, list);
	}
	g_list_free (list);
	return counts;
}

gint  ugtk_traveler_move_selected_bottom (UgtkTraveler* traveler)
{
	UgetNode* node;
	UgetNode* sibling;
	UgetNode* bottom;
	GList*    list;
	GList*    link;
	int       counts = 0;

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
		if (sibling->next == node)
			continue;
		uget_node_move (node->parent, sibling->next, node);
		counts++;
	}

	if (counts > 0) {
		ugtk_node_tree_refresh (traveler->download.model);
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
	GList*  selected;

	if (nth_col >= UGTK_NODE_N_COLUMNS)
		return;

	selected = ugtk_traveler_get_selected (traveler);
	if (nth_col <= 0)
		uget_app_set_sorting ((UgetApp*) traveler->app, NULL, FALSE);
	else {
		uget_app_set_sorting ((UgetApp*) traveler->app, compare_funcs[nth_col],
				(type == GTK_SORT_DESCENDING) ? TRUE : FALSE);
	}
	ugtk_traveler_set_selected (traveler, selected);
	g_list_free (selected);
	gtk_widget_queue_draw (GTK_WIDGET (traveler->download.view));
}

// ----------------------------------------------------------------------------
// signal handlers

static void on_state_selection_changed (GtkSelectionModel* model, guint pos, guint n_items, UgtkTraveler* traveler)
{
	guint selected;

	// clear download cursor
	traveler->download.cursor.node = NULL;

	traveler->state.cursor.pos_last = traveler->state.cursor.pos;
	selected = gtk_single_selection_get_selected (traveler->state.selection);
	if (selected == GTK_INVALID_LIST_POSITION) {
		traveler->state.cursor.pos = -1;
		traveler->state.cursor.node = NULL;
		return;
	}
	if (traveler->state.cursor.pos == (int)selected)
		return;

	traveler->state.cursor.pos = selected;
	traveler->state.cursor.node = get_node_at (
			G_LIST_MODEL (traveler->state.model), selected);

	// change download.model root and refresh
	if (traveler->state.cursor.node) {
		traveler->download.model->root = traveler->state.cursor.node;
		ugtk_node_tree_refresh (traveler->download.model);
	}
}

static void on_category_selection_changed (GtkSelectionModel* model, guint pos, guint n_items, UgtkTraveler* traveler)
{
	guint selected;
	UgetNode* node;

	// clear download cursor
	traveler->download.cursor.node = NULL;

	traveler->category.cursor.pos_last = traveler->category.cursor.pos;
	selected = gtk_single_selection_get_selected (traveler->category.selection);
	if (selected == GTK_INVALID_LIST_POSITION) {
		traveler->category.cursor.pos = -1;
		traveler->category.cursor.node = NULL;
		return;
	}
	if (traveler->category.cursor.pos == (int)selected)
		return;

	traveler->category.cursor.pos = selected;
	node = get_node_at (G_LIST_MODEL (traveler->category.model), selected);
	traveler->category.cursor.node = node;

	// change state.model root and refresh
	if (node) {
		traveler->state.model->root = node;
		ugtk_node_list_refresh (traveler->state.model);
		// re-select the same state position
		if (traveler->state.cursor.pos >= 0)
			gtk_single_selection_set_selected (traveler->state.selection,
					traveler->state.cursor.pos);
	}
}

static void on_download_selection_changed (GtkSelectionModel* model, guint pos, guint n_items, UgtkTraveler* traveler)
{
	GtkBitset*    bitset;
	guint         first;

	traveler->download.cursor.pos_last = traveler->download.cursor.pos;

	bitset = gtk_selection_model_get_selection (model);
	if (gtk_bitset_is_empty (bitset)) {
		traveler->download.cursor.pos = -1;
		traveler->download.cursor.node = NULL;
		gtk_bitset_unref (bitset);
		return;
	}

	first = gtk_bitset_get_nth (bitset, 0);
	traveler->download.cursor.pos = first;
	traveler->download.cursor.node = get_node_at (
			G_LIST_MODEL (traveler->download.model), first);
	gtk_bitset_unref (bitset);
}

// ----------------------------------------------------------------------------
// static data

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
