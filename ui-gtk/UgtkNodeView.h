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

#ifndef UGTK_NODE_VIEW_H
#define UGTK_NODE_VIEW_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

// Column indices for the download GtkColumnView
typedef enum
{
	UGTK_NODE_COLUMN_STATE    = 0,
	UGTK_NODE_COLUMN_NAME     = 1,

	// Category & Status only
	UGTK_NODE_COLUMN_QUANTITY = 2,

	// Download only
	UGTK_NODE_COLUMN_COMPLETE = 2, // complete size
	UGTK_NODE_COLUMN_TOTAL,        // total size
	UGTK_NODE_COLUMN_PERCENT,
	UGTK_NODE_COLUMN_ELAPSED,      // consuming time
	UGTK_NODE_COLUMN_LEFT,         // remaining time
	UGTK_NODE_COLUMN_SPEED,
	UGTK_NODE_COLUMN_UPLOAD_SPEED, // torrent
	UGTK_NODE_COLUMN_UPLOADED,     // torrent
	UGTK_NODE_COLUMN_RATIO,        // torrent
	UGTK_NODE_COLUMN_RETRY,
	UGTK_NODE_COLUMN_CATEGORY,     // category name
	UGTK_NODE_COLUMN_URI,
	UGTK_NODE_COLUMN_ADDED_ON,
	UGTK_NODE_COLUMN_COMPLETED_ON,

	UGTK_NODE_N_COLUMNS,
} UgtkNodeColumn;

// return GtkColumnView (download) or GtkListView (category/state)
GtkWidget*  ugtk_node_view_new_for_download (void);
GtkWidget*  ugtk_node_view_new_for_category (void);
GtkWidget*  ugtk_node_view_new_for_state (void);

#ifdef __cplusplus
}
#endif

#endif // UGTK_NODE_VIEW_H
