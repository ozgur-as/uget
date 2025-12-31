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

#ifndef UGTK_SUMMARY_H
#define UGTK_SUMMARY_H

#include <gtk/gtk.h>
#include <UgetNode.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef	struct	UgtkSummary            UgtkSummary;

enum UGTK_SUMMARY_COLUMN
{
	UGTK_SUMMARY_COLUMN_ICON,
	UGTK_SUMMARY_COLUMN_NAME,
	UGTK_SUMMARY_COLUMN_VALUE,
	UGTK_SUMMARY_N_COLUMN
};

struct UgtkSummary
{
	GtkWidget*      self;    // (GtkScrolledWindow) container for view
	GtkTreeView*    view;
	GtkListStore*   store;

	struct
	{
		GtkWidget*  self;       // (GtkPopover) pop-up menu
		GtkWidget*  copy;       // GtkMenuItem
		GtkWidget*  copy_all;   // GtkMenuItem
	} menu;

	struct
	{
		gboolean    name:1;
		gboolean    folder:1;
		gboolean    category:1;
		gboolean    uri:1;
		gboolean    message:1;
	} visible;
};

void  ugtk_summary_init (UgtkSummary* summary, gpointer accel_group);
void  ugtk_summary_show (UgtkSummary* summary, UgetNode* node);

// call g_free() to free returned string.
gchar* ugtk_summary_get_text_selected (UgtkSummary* summary);
gchar* ugtk_summary_get_text_all (UgtkSummary* summary);


#ifdef __cplusplus
}
#endif

#endif  // End of UGTK_SUMMARY_H


