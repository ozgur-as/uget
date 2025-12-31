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

#ifndef UGTK_MENUBAR_H
#define UGTK_MENUBAR_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UgtkMenubar       UgtkMenubar;
typedef struct UgtkApp           UgtkApp;

struct UgtkMenubar
{
	GtkWidget*  self;   // GtkMenuBar

	// GtkWidget*  self;    // GtkMenu*
	// GtkWidget*  shell;   // GtkMenuShell*
	// GtkWidget*  other;   // GtkMenuItem*
	struct UgtkFileMenu
	{
		// file.create
		GtkWidget*  create_download;
		GtkWidget*  create_category;
		GtkWidget*  create_torrent;
		GtkWidget*  create_metalink;

		// file.batch
		struct UgtkFileBatchMenu
		{
			GtkWidget*  clipboard;    // Clipboard batch
			GtkWidget*  sequence;     // URL Sequence batch
			GtkWidget*  text_import;  // Text file import (.txt)
			GtkWidget*  html_import;  // HTML file import (.html)
			GtkWidget*  text_export;  // Export to Text file (.txt)
		} batch;

		GtkWidget*  open_category;
		GtkWidget*  save_category;
		GtkWidget*  save;
		GtkWidget*  offline_mode;
		GtkWidget*  quit;
	} file;

	struct UgtkEditMenu
	{
		GtkWidget*  clipboard_monitor;
		GtkWidget*  clipboard_quiet;
		GtkWidget*  commandline_quiet;
		GtkWidget*  skip_existing;
		GtkWidget*  apply_recent;
		GtkWidget*  settings;

		// Completion Auto-Actions
		struct {
			GtkWidget*  disable;
			GtkWidget*  hibernate;
			GtkWidget*  suspend;
			GtkWidget*  shutdown;
			GtkWidget*  reboot;
			GtkWidget*  custom;
			// separator
			GtkWidget*  remember;
			GtkWidget*  help;
		} completion;
	} edit;

	struct UgtkViewMenu
	{
		GtkWidget*  toolbar;
		GtkWidget*  statusbar;
		GtkWidget*  category;
		GtkWidget*  summary;

		struct UgtkViewItemMenu
		{
			GtkWidget*  name;
			GtkWidget*  folder;
			GtkWidget*  category;
//			GtkWidget*  elapsed;
			GtkWidget*  uri;
			GtkWidget*  message;
		} summary_items;

		struct UgtkViewColMenu    // download columns
		{
			GtkWidget*  self;    // GtkMenu

			GtkWidget*  complete;
			GtkWidget*  total;
			GtkWidget*  percent;
			GtkWidget*  elapsed;    // consuming time
			GtkWidget*  left;       // remaining time
			GtkWidget*  speed;
			GtkWidget*  upload_speed;    // torrent
			GtkWidget*  uploaded;        // torrent
			GtkWidget*  ratio;           // torrent
			GtkWidget*  retry;
			GtkWidget*  category;
			GtkWidget*  uri;
			GtkWidget*  added_on;
			GtkWidget*  completed_on;
		} columns;
	} view;

	struct UgtkCategoryMenu
	{
		GtkWidget*  self;    // GtkMenu

		GtkWidget*  create;
		GtkWidget*  delete;
		GtkWidget*  properties;
		GtkWidget*  move_up;
		GtkWidget*  move_down;
	} category;

	struct UgtkDownloadMenu
	{
		GtkWidget*  self;    // GtkMenu

		GtkWidget*  create;
		GtkWidget*  delete;
		GtkWidget*  delete_file;    // delete file and data.
		GtkWidget*  open;
		GtkWidget*  open_folder;    // open containing folder
		GtkWidget*  force_start;
		GtkWidget*  runnable;
		GtkWidget*  pause;

		struct UgtkDownloadMoveToMenu
		{
			GtkWidget*  self;    // GtkMenu
			GtkWidget*  item;    // GtkMenuItem

			// This array used for mapping menu item and it's category
			// index 0, 2, 4, 6...	GtkMenuItem*
			// index 1, 3, 5, 7...	UgetNode*
			GPtrArray*  array;
		} move_to;

		GtkWidget*  move_up;
		GtkWidget*  move_down;
		GtkWidget*  move_top;
		GtkWidget*  move_bottom;

		struct UgtkDownloadPrioriyMenu
		{
			GtkWidget*  self;    // GtkMenu
			GtkWidget*  item;    // GtkMenuItem
			// GtkRadioMenuItem
			GtkWidget*  high;
			GtkWidget*  normal;
			GtkWidget*  low;
		} prioriy;

		GtkWidget*  properties;
	} download;

	struct UgtkHelpMenu
	{
		GtkWidget*  help_online;
		GtkWidget*  documentation;
		GtkWidget*  support_forum;
		GtkWidget*  submit_feedback;
		GtkWidget*  report_bug;
		GtkWidget*  keyboard_shortcuts;
		GtkWidget*  check_updates;
		GtkWidget*  about_uget;
	} help;
};

void  ugtk_menubar_init_ui (UgtkMenubar* menubar, gpointer accel_group);
void  ugtk_menubar_init_callback (UgtkMenubar* menubar, UgtkApp* app);
void  ugtk_menubar_register_actions (UgtkApp *app);
void  ugtk_context_menus_init (UgtkApp *app);

void  ugtk_menubar_sync_category (UgtkMenubar* menubar, UgtkApp* app, gboolean reset);

#ifdef __cplusplus
}
#endif

#endif // UGTK_MENUBAR_H
