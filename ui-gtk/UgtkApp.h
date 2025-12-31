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

#ifndef UGTK_APP_H
#define UGTK_APP_H

#include <gtk/gtk.h>
#include <UgetApp.h>
#include <UgetRpc.h>
#include <UgetRss.h>
#include <UgtkSetting.h>
#include <UgtkTrayIcon.h>
#include <UgtkBanner.h>
#include <UgtkMenubar.h>
#include <UgtkSummary.h>
#include <UgtkTraveler.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UGTK_APP_DIR         "uGet"
#define UGTK_APP_NAME        "uGet"
#define UGTK_APP_ICON_NAME   "uget-icon"

#define	UGTK_APP_ACCEL_PATH_NEW         "<uGet>/File/New"
#define	UGTK_APP_ACCEL_PATH_LOAD        "<uGet>/File/Load"
#define	UGTK_APP_ACCEL_PATH_SAVE        "<uGet>/File/Save"
#define	UGTK_APP_ACCEL_PATH_SAVE_ALL    "<uGet>/File/SaveAll"
#define	UGTK_APP_ACCEL_PATH_DELETE      "<uGet>/Download/Delete"
#define	UGTK_APP_ACCEL_PATH_DELETE_F    "<uGet>/Download/DeleteFile"
#define	UGTK_APP_ACCEL_PATH_OPEN        "<uGet>/Download/Open"
#define	UGTK_APP_ACCEL_PATH_OPEN_F      "<uGet>/Download/OpenFolder"
#define	UGTK_APP_ACCEL_PATH_SWITCH      "<uGet>/Download/SwitchState"

typedef struct UgtkApp        UgtkApp;

struct UgtkApp
{
	UGET_APP_MEMBERS;
/*	// ------ UgetApp members ------
	UgetNode        real;           // real root node for real nodes
	UgetNode        split;          // virtual root
	UgetNode        sorted;         // virtual root
	UgetNode        sorted_split;   // virtual root
	UgetNode        mix;            // virtual root
	UgetNode        mix_split;      // virtual root
	UgRegistry      infos;
	UgRegistry      plugins;
	UgetPluginInfo* plugin_default;
	UgetTask        task;
	UgArrayPtr      nodes;
	void*           uri_hash;
	char*           config_dir;
	int             n_error;        // uget_app_grow() will count these value:
	int             n_moved;        // n_error, n_moved, n_deleted, and
	int             n_deleted;      // n_completed
	int             n_completed;    //
 */

	UgetRpc*          rpc;
	GSimpleActionGroup*  action_group;  // Menu action group for GAction state updates
	UgtkSetting       setting;
	UgtkScheduleState schedule_state;

	// recent download settings
	struct {
		UgInfo*     info;
		int         category_index;
		int         saved;
	} recent;
	// status
	gboolean        user_action;

	// RSS
	UgetRss*        rss_builtin;  // Built-in RSS

	// Clipboard
	struct UgtkClipboard
	{
		GdkClipboard*  self;
		gchar*         text;
		GRegex*        regex;
		gboolean       processing;
		// monitor storage or media website
		gboolean       website;
	} clipboard;

	// dialogs
	struct UgtkDialogs
	{
		GtkWidget*     error;
		GtkWidget*     message;
		GtkWidget*     exit_confirmation;
		GtkWidget*     delete_confirmation;
		GtkWidget*     delete_category_confirmation;
		GtkWidget*     setting;
	} dialogs;

	// -------------------------------------------------------
	// GUI: Main Window and Tray Icon
	gpointer         accel_group;
	UgtkTrayIcon     trayicon;
	UgtkBanner       banner;
	UgtkMenubar      menubar;
	UgtkSummary      summary;
	UgtkTraveler     traveler;   // (UgetNode) node traveler

	// Context menus (GtkPopoverMenu for GTK4)
	GtkWidget*       category_context_menu;
	GtkWidget*       download_context_menu;
	GtkWidget*       summary_context_menu;
	GMenu*           menubar_move_to_menu;  // Move To submenu in menubar for dynamic population

	// --------------------------------
	// Main Window (initialize in UgtkApp-ui.c)
	struct UgtkWindow
	{
		GtkWindow*   self;
		GtkPaned*    hpaned;    // separate left side and right side
		GtkPaned*    vpaned;    // right side (UgtkTreeView and UgtkSummary)
	} window;

	// --------------------------------
	// status bar (initialize in UgtkApp-ui.c)
	struct UgtkStatusbar
	{
		GtkStatusbar*  self;
		GtkLabel*      down_speed;
		GtkLabel*      up_speed;
	} statusbar;

	// --------------------------------
	// Toolbar (initialize in UgtkApp-ui.c)
	struct UgtkToolbar
	{
		GtkWidget*  self;    // GtkBox
		GtkWidget*  toolbar;

		// GtkToolItem
		GtkWidget*  create;
		// GtkMenuItem
		// menu for tool button
		GtkWidget*  create_download;
		GtkWidget*  create_category;
		GtkWidget*  create_clipboard;
		GtkWidget*  create_sequence;
		GtkWidget*  create_torrent;
		GtkWidget*  create_metalink;

		// GtkToolItem
		GtkWidget*  save;
		GtkWidget*  runnable;
		GtkWidget*  pause;
		GtkWidget*  properties;

		GtkWidget*  move_up;
		GtkWidget*  move_down;
		GtkWidget*  move_top;
		GtkWidget*  move_bottom;
	} toolbar;
};

void  ugtk_app_init  (UgtkApp* app, UgetRpc* ipc);
void  ugtk_app_final (UgtkApp* app);

// UgtkApp-ui.c, UgtkApp-callback.c, and UgtkApp-timeout.c
void  ugtk_app_init_ui (UgtkApp* app);
void  ugtk_app_init_callback (UgtkApp* app);
void  ugtk_app_init_timeout (UgtkApp* app);

void  ugtk_app_save (UgtkApp* app);
void  ugtk_app_load (UgtkApp* app);
void  ugtk_app_quit (UgtkApp* app);

// set UgtkSetting from/to UgtkApp
void  ugtk_app_get_window_setting (UgtkApp* app, UgtkSetting* setting);
void  ugtk_app_set_window_setting (UgtkApp* app, UgtkSetting* setting);
void  ugtk_app_get_column_setting (UgtkApp* app, UgtkSetting* setting);
void  ugtk_app_set_column_setting (UgtkApp* app, UgtkSetting* setting);
void  ugtk_app_set_plugin_setting (UgtkApp* app, UgtkSetting* setting);
void  ugtk_app_set_other_setting (UgtkApp* app, UgtkSetting* setting);
void  ugtk_app_set_menu_setting (UgtkApp* app, UgtkSetting* setting);
void  ugtk_app_set_ui_setting (UgtkApp* app, UgtkSetting* setting);

// decide sensitive or visible
void  ugtk_app_decide_download_sensitive (UgtkApp* app);
void  ugtk_app_decide_category_sensitive (UgtkApp* app);
void  ugtk_app_decide_trayicon_visible (UgtkApp* app);
void  ugtk_app_decide_to_quit (UgtkApp* app);

// create node by UI
void  ugtk_app_create_category (UgtkApp* app);
void  ugtk_app_create_download (UgtkApp* app, const char* sub_title, const char* uri);
// delete selected node
void  ugtk_app_delete_category (UgtkApp* app);
void  ugtk_app_delete_download (UgtkApp* app, gboolean delete_files);
// edit selected node
void  ugtk_app_edit_category (UgtkApp* app);
void  ugtk_app_edit_download (UgtkApp* app);
// queue/pause
void  ugtk_app_queue_download (UgtkApp* app, gboolean keep_active);
void  ugtk_app_pause_download (UgtkApp* app);
void  ugtk_app_switch_download_state (UgtkApp* app);
// move selected node
void  ugtk_app_move_download_up (UgtkApp* app);
void  ugtk_app_move_download_down (UgtkApp* app);
void  ugtk_app_move_download_top (UgtkApp* app);
void  ugtk_app_move_download_bottom (UgtkApp* app);
void  ugtk_app_move_download_to (UgtkApp* app, UgetNode* cnode);
// torrent & metalink
void  ugtk_app_create_torrent (UgtkApp* app);
void  ugtk_app_create_metalink (UgtkApp* app);
// import/export
void  ugtk_app_save_category (UgtkApp* app);
void  ugtk_app_load_category (UgtkApp* app);
void  ugtk_app_import_html_file (UgtkApp* app);
void  ugtk_app_import_text_file (UgtkApp* app);
void  ugtk_app_export_text_file (UgtkApp* app);
// batch
void  ugtk_app_sequence_batch (UgtkApp* app);
void  ugtk_app_clipboard_batch (UgtkApp* app);
int   ugtk_app_filter_existing (UgtkApp* app, GList* uris);

// emit signal "row-changed" for UgtkNodeTree and UgtkNodeList
void  ugtk_app_download_changed  (UgtkApp* app, UgetNode* dnode);
void  ugtk_app_category_changed  (UgtkApp* app, UgetNode* cnode);

void  ugtk_app_add_default_category (UgtkApp* app);

void  ugtk_app_show_message (UgtkApp* app, GtkMessageType type,
                             const gchar* message);

void  ugtk_app_open_settings (UgtkApp* app);

// --------------------------------------------------------
// UgClipboard
void  ugtk_clipboard_init (struct UgtkClipboard* clipboard, const gchar* pattern);
void  ugtk_clipboard_set_pattern (struct UgtkClipboard* clipboard, const gchar* pattern);
void  ugtk_clipboard_set_text (struct UgtkClipboard* clipboard, gchar* text);
GList* ugtk_clipboard_get_uris (struct UgtkClipboard* clipboard);
GList* ugtk_clipboard_get_matched (struct UgtkClipboard* clipboard, const gchar* text);

// --------------------------------------------------------
// UgStatusbar
void  ugtk_statusbar_set_info (struct UgtkStatusbar* statusbar, gint n_selected);
void  ugtk_statusbar_set_speed (struct UgtkStatusbar* statusbar, gint64 down_speed, gint64 up_speed);

#ifdef __cplusplus
}
#endif

#endif // UGTK_APP_H
