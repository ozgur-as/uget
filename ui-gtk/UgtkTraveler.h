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

#ifndef UGTK_TRAVELER_H
#define UGTK_TRAVELER_H

#include <gtk/gtk.h>
#include <UgtkNodeList.h>
#include <UgtkNodeTree.h>
#include <UgtkNodeView.h>

#ifdef __cplusplus
extern "C" {
#endif

// Uget GTK node Traveler
typedef struct UgtkTraveler    UgtkTraveler;
typedef struct UgtkApp         UgtkApp;

struct UgtkTraveler
{
	UgtkApp*  app;

	struct {
		GtkWidget*          self;       // GtkListView
		GtkListView*        view;
		UgtkNodeList*       model;
		GtkSingleSelection* selection;

		struct {
			int       pos;
			int       pos_last;
			UgetNode* node;
		} cursor;
	} state;

	struct {
		GtkWidget*          self;       // GtkScrolledWindow
		GtkListView*        view;
		UgtkNodeTree*       model;
		GtkSingleSelection* selection;

		struct {
			int       pos;
			int       pos_last;
			UgetNode* node;
		} cursor;
	} category;

	struct {
		GtkWidget*          self;       // GtkScrolledWindow
		GtkColumnView*      view;
		UgtkNodeTree*       model;
		GtkMultiSelection*  selection;

		struct {
			int       pos;
			int       pos_last;
			UgetNode* node;
		} cursor;
	} download;

	// ugtk_traveler_reserve_selection()
	// ugtk_traveler_restore_selection()
	struct {
		GList*    list;
		UgetNode* node;
	} reserved;
};


void  ugtk_traveler_init (UgtkTraveler* traveler, UgtkApp* app);

void  ugtk_traveler_select_category (UgtkTraveler* traveler,
                                     int nth_category, int nth_state);

// get download cursor
void      ugtk_traveler_set_cursor (UgtkTraveler* traveler, UgetNode* dnode);
UgetNode* ugtk_traveler_get_cursor (UgtkTraveler* traveler);

// return all selected download nodes (reverse order)
GList* ugtk_traveler_get_selected (UgtkTraveler* traveler);
void   ugtk_traveler_set_selected (UgtkTraveler* traveler,
                                   GList*        nodes);

GList* ugtk_traveler_reserve_selection (UgtkTraveler* traveler);
void   ugtk_traveler_restore_selection (UgtkTraveler* traveler);

gint  ugtk_traveler_move_selected_up (UgtkTraveler* traveler);
gint  ugtk_traveler_move_selected_down (UgtkTraveler* traveler);
gint  ugtk_traveler_move_selected_top (UgtkTraveler* traveler);
gint  ugtk_traveler_move_selected_bottom (UgtkTraveler* traveler);

void  ugtk_traveler_set_sorting (UgtkTraveler*  traveler,
                                 gboolean       user_sortable,
                                 UgtkNodeColumn nth_column,
                                 GtkSortType    type);

#ifdef __cplusplus
}
#endif

#endif // UGTK_TRAVELER_H
