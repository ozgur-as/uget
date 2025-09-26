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

#include <UgUtil.h>
#include <UgStdio.h>
#include <UgString.h>
#include <UgFileUtil.h>
#include <UgHtmlFilter.h>
#include <UgetPluginCurl.h>
#include <UgetPluginAria2.h>
#include <UgetPluginMedia.h>
#include <UgetPluginMega.h>
#include <UgtkApp.h>
#include <UgtkUtil.h>
#include <UgtkNodeDialog.h>
#include <UgtkBatchDialog.h>
#include <UgtkConfirmDialog.h>

#include <glib/gi18n.h>

void  ugtk_app_init (UgtkApp* app, UgetRpc* rpc)
{
	char*  dir;

	app->rpc = rpc;
	uget_app_init ((UgetApp*) app);
	// set application config directory for each user
	dir = g_build_filename (ugtk_get_config_dir (), UGTK_APP_DIR, NULL);
	uget_app_set_config_dir ((UgetApp*) app, dir);
	g_free (dir);

	app->rss_builtin = uget_rss_new ();
	ugtk_app_load (app);
	ugtk_app_init_ui (app);
	ugtk_app_init_callback (app);
	if (app->real.n_children == 0)
		ugtk_app_add_default_category (app);
	// clipboard
	ugtk_clipboard_init (&app->clipboard, app->setting.clipboard.pattern);
	// plug-in initialize
	uget_plugin_global_set(UgetPluginCurlInfo,  UGET_PLUGIN_GLOBAL_INIT, (void*) TRUE);
	uget_plugin_global_set(UgetPluginAria2Info, UGET_PLUGIN_GLOBAL_INIT, (void*) TRUE);
        uget_plugin_global_set(UgetPluginMediaInfo, UGET_PLUGIN_GLOBAL_INIT, (void*) TRUE);
#ifdef HAVE_MEGA
        uget_plugin_global_set(UgetPluginMegaInfo,  UGET_PLUGIN_GLOBAL_INIT, (void*) TRUE);
#endif
	// apply UgtkSetting
	ugtk_app_set_plugin_setting (app, &app->setting);
	ugtk_app_set_window_setting (app, &app->setting);
	ugtk_app_set_column_setting (app, &app->setting);
	ugtk_app_set_other_setting (app, &app->setting);
	ugtk_app_set_ui_setting (app, &app->setting);

	ugtk_tray_icon_set_info (&app->trayicon, 0, 0, 0);
	ugtk_statusbar_set_speed (&app->statusbar, 0, 0);
	ugtk_menubar_sync_category (&app->menubar, app, TRUE);

	app->recent.category_index = 0;
	app->recent.info = ug_info_new(8, 0);
	// RSS
	uget_rss_add_builtin (app->rss_builtin, UGET_RSS_STABLE);
	uget_rss_add_builtin (app->rss_builtin, UGET_RSS_NEWS);
	uget_rss_add_builtin (app->rss_builtin, UGET_RSS_TUTORIALS);
	uget_rss_update (app->rss_builtin, FALSE);
	gtk_widget_hide (app->banner.self);

	uget_app_use_uri_hash ((UgetApp*) app);
	ugtk_app_init_timeout (app);

	if (app->setting.ui.start_in_tray)
		ugtk_tray_icon_set_visible (&app->trayicon, TRUE);
	else
		gtk_widget_show ((GtkWidget*) app->window.self);
	// offline
	if (app->setting.ui.start_in_offline_mode)
		g_signal_emit_by_name (app->menubar.file.offline_mode, "activate");
}

void  ugtk_app_final (UgtkApp* app)
{
	int  shutdown_now;

	uget_app_set_notification ((UgetApp*) app, NULL, NULL, NULL, NULL);

	if (app->setting.plugin_order >= UGTK_PLUGIN_ORDER_ARIA2)
		shutdown_now = app->setting.aria2.shutdown;
	else
		shutdown_now = FALSE;
	ug_info_unref(app->recent.info);
	uget_rss_unref (app->rss_builtin);
	uget_app_final ((UgetApp*) app);
	// plug-in finalize
	uget_plugin_global_set(UgetPluginAria2Info, UGET_PLUGIN_ARIA2_GLOBAL_SHUTDOWN_NOW,
			(void*)(intptr_t) shutdown_now);
	uget_plugin_global_set(UgetPluginCurlInfo,  UGET_PLUGIN_GLOBAL_INIT, (void*) FALSE);
	uget_plugin_global_set(UgetPluginAria2Info, UGET_PLUGIN_GLOBAL_INIT, (void*) FALSE);
        uget_plugin_global_set(UgetPluginMediaInfo, UGET_PLUGIN_GLOBAL_INIT, (void*) FALSE);
#ifdef HAVE_MEGA
        uget_plugin_global_set(UgetPluginMegaInfo,  UGET_PLUGIN_GLOBAL_INIT, (void*) FALSE);
#endif
}

void  ugtk_app_save (UgtkApp* app)
{
	gchar*    file;

	if (app->config_dir == NULL)
		return;
	ug_create_dir_all (app->config_dir, -1);
	file = g_build_filename (app->config_dir, "Setting.json", NULL);
	ugtk_setting_save (&app->setting, file);
	g_free (file);

	// RSS
	file = g_build_filename (app->config_dir, "RSS-built-in.json", NULL);
	uget_rss_save_feeds (app->rss_builtin, file);
	g_free (file);

//	uget_app_save_categories ((UgetApp*) app, ugtk_get_config_dir ());
	uget_app_save_categories ((UgetApp*) app, NULL);
}

void  ugtk_app_load (UgtkApp* app)
{
	int       counts;
	gchar*    file;

	// load setting
	ugtk_setting_init (&app->setting);
	file = g_build_filename (app->config_dir, "Setting.json", NULL);
	counts = ugtk_setting_load (&app->setting, file);
	g_free (file);
	if (counts == FALSE)
		ugtk_setting_reset (&app->setting);
	else if (app->setting.scheduler.state.length < 7*24) {
		ug_array_alloc (&app->setting.scheduler.state,
		                7*24 - app->setting.scheduler.state.length);
	}

	// RSS
	file = g_build_filename (app->config_dir, "RSS-built-in.json", NULL);
	uget_rss_load_feeds (app->rss_builtin, file);
	g_free (file);

//	uget_app_load_categories ((UgetApp*) app, ugtk_get_config_dir ());
	counts = uget_app_load_categories ((UgetApp*) app, NULL);
	if (counts == 0)
		ugtk_app_add_default_category (app);
}

void  ugtk_app_quit (UgtkApp* app)
{
	// stop all tasks
	uget_task_remove_all (&app->task);
	// sync setting and save data
	ugtk_app_get_window_setting (app, &app->setting);
	ugtk_app_get_column_setting (app, &app->setting);
	ugtk_app_save (app);
	// clear plug-in
	uget_app_clear_plugins ((UgetApp*) app);
	// hide icon in system tray before quit
	ugtk_tray_icon_set_visible (&app->trayicon, FALSE);
	// hide window
	gtk_widget_hide (GTK_WIDGET (app->window.self));

	gtk_main_quit ();
}

void  ugtk_app_get_window_setting (UgtkApp* app, UgtkSetting* setting)
{
	GdkWindowState  gdk_wstate;
	GdkWindow*      gdk_window;
	gint            x, y;

	// get window position, size, and maximized state
	if (gtk_widget_get_visible (GTK_WIDGET (app->window.self)) == TRUE) {
		gdk_window = gtk_widget_get_window (GTK_WIDGET (app->window.self));
		gdk_wstate = gdk_window_get_state (gdk_window);

		if (gdk_wstate & GDK_WINDOW_STATE_MAXIMIZED)
			setting->window.maximized = TRUE;
		else
			setting->window.maximized = FALSE;
		// get geometry
		if (setting->window.maximized == FALSE) {
			gtk_window_get_position (app->window.self, &x, &y);
			gtk_window_get_size (app->window.self,
					&setting->window.width, &setting->window.height);
			// gtk_window_get_position() may return: x == -32000, y == -32000
			if (x + app->setting.window.width > 0)
				setting->window.x = x;
			if (y + app->setting.window.height > 0)
				setting->window.y = y;
		}
	}
	// GtkPaned position
	if (app->setting.window.category)
		setting->window.paned_position_h = gtk_paned_get_position (app->window.hpaned);
	if (app->setting.window.summary)
		setting->window.paned_position_v = gtk_paned_get_position (app->window.vpaned);

	// banner
	setting->window.banner = gtk_widget_get_visible (app->banner.self);
	// traveler
	setting->window.nth_category = app->traveler.category.cursor.pos;
	setting->window.nth_state    = app->traveler.state.cursor.pos;
}

void  ugtk_app_set_window_setting (UgtkApp* app, UgtkSetting* setting)
{
	// set window position, size, and maximized state
	if (setting->window.width  > 0 &&
	    setting->window.height > 0 &&
	    setting->window.x < gdk_screen_width ()  &&
	    setting->window.y < gdk_screen_height () &&
	    setting->window.x + setting->window.width > 0  &&
	    setting->window.y + setting->window.height > 0)
	{
		gtk_window_move (app->window.self,
				setting->window.x, setting->window.y);
		gtk_window_resize (app->window.self,
				setting->window.width, setting->window.height);
	}
	if (setting->window.maximized)
		gtk_window_maximize (app->window.self);
	// GtkPaned position
	if (setting->window.paned_position_h > 0) {
		gtk_paned_set_position (app->window.hpaned,
		                        setting->window.paned_position_h);
	}
	if (setting->window.paned_position_v > 0) {
		if (setting->window.paned_position_v > 100) // for uGet < 2.0.4
		gtk_paned_set_position (app->window.vpaned,
		                        setting->window.paned_position_v);
	}
	// set visible widgets
	gtk_widget_set_visible (app->toolbar.self,
			setting->window.toolbar);
	gtk_widget_set_visible ((GtkWidget*) app->statusbar.self,
			setting->window.statusbar);
	gtk_widget_set_visible (gtk_paned_get_child1 (app->window.hpaned),
			setting->window.category);
	gtk_widget_set_visible (app->summary.self,
			setting->window.summary);
	gtk_widget_set_visible (app->banner.self,
			setting->window.banner);
	// Summary
	app->summary.visible.name     = setting->summary.name;
	app->summary.visible.folder   = setting->summary.folder;
	app->summary.visible.category = setting->summary.category;
	app->summary.visible.uri      = setting->summary.uri;
	app->summary.visible.message  = setting->summary.message;

	// traveler
	if (setting->window.nth_category >= app->real.n_children)
		setting->window.nth_category = 0;
	if (setting->window.nth_state >= app->split.n_children)
		setting->window.nth_state = 0;
	ugtk_traveler_select_category (&app->traveler,
	                               setting->window.nth_category,
	                               setting->window.nth_state);
	// menu
	ugtk_app_set_menu_setting (app, setting);
}

void  ugtk_app_get_column_setting (UgtkApp* app, UgtkSetting* setting)
{
	GtkTreeViewColumn* column;
	int                width;

	// state
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_STATE);
	width = gtk_tree_view_column_get_width (column);
	setting->download_column.width.state = width;
	// name
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_NAME);
	width = gtk_tree_view_column_get_width (column);
	setting->download_column.width.name = width;
	// complete
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_COMPLETE);
	width = gtk_tree_view_column_get_width (column);
	setting->download_column.width.complete = width;
	// total
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_TOTAL);
	width = gtk_tree_view_column_get_width (column);
	setting->download_column.width.total = width;
	// percent
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_PERCENT);
	width = gtk_tree_view_column_get_width (column);
	setting->download_column.width.percent = width;
	// elapsed
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_ELAPSED);
	width = gtk_tree_view_column_get_width (column);
	setting->download_column.width.elapsed = width;
	// left
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_LEFT);
	width = gtk_tree_view_column_get_width (column);
	setting->download_column.width.left = width;
	// speed
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_SPEED);
	width = gtk_tree_view_column_get_width (column);
	setting->download_column.width.speed = width;
	// upload_speed
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_UPLOAD_SPEED);
	width = gtk_tree_view_column_get_width (column);
	setting->download_column.width.upload_speed = width;
	// uploaded
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_UPLOADED);
	width = gtk_tree_view_column_get_width (column);
	setting->download_column.width.uploaded = width;
	// ratio
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_RATIO);
	width = gtk_tree_view_column_get_width (column);
	setting->download_column.width.ratio = width;
	// retry
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_RETRY);
	width = gtk_tree_view_column_get_width (column);
	setting->download_column.width.retry = width;
	// category
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_CATEGORY);
	width = gtk_tree_view_column_get_width (column);
	setting->download_column.width.category = width;
	// uri
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_URI);
	width = gtk_tree_view_column_get_width (column);
	setting->download_column.width.uri = width;
	// added_on
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_ADDED_ON);
	width = gtk_tree_view_column_get_width (column);
	setting->download_column.width.added_on = width;
	// completed_on
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_COMPLETED_ON);
	width = gtk_tree_view_column_get_width (column);
	setting->download_column.width.completed_on = width;
}

void  ugtk_app_set_column_setting (UgtkApp* app, UgtkSetting* setting)
{
	GtkTreeViewColumn* column;
	int                width;

	// state
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_STATE);
	width = setting->download_column.width.state;
	if (width > 0)
		gtk_tree_view_column_set_fixed_width (column, width);
	// name
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_NAME);
	width = setting->download_column.width.name;
	if (width > 0)
		gtk_tree_view_column_set_fixed_width (column, width);
	// complete
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_COMPLETE);
	width = setting->download_column.width.complete;
	if (width > 0)
		gtk_tree_view_column_set_fixed_width (column, width);
	// total
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_TOTAL);
	width = setting->download_column.width.total;
	if (width > 0)
		gtk_tree_view_column_set_fixed_width (column, width);
	// percent
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_PERCENT);
	width = setting->download_column.width.percent;
	if (width > 0)
		gtk_tree_view_column_set_fixed_width (column, width);
	// elapsed
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_ELAPSED);
	width = setting->download_column.width.elapsed;
	if (width > 0)
		gtk_tree_view_column_set_fixed_width (column, width);
	// left
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_LEFT);
	width = setting->download_column.width.left;
	if (width > 0)
		gtk_tree_view_column_set_fixed_width (column, width);
	// speed
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_SPEED);
	width = setting->download_column.width.speed;
	if (width > 0)
		gtk_tree_view_column_set_fixed_width (column, width);
	// upload_speed
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_UPLOAD_SPEED);
	width = setting->download_column.width.upload_speed;
	if (width > 0)
		gtk_tree_view_column_set_fixed_width (column, width);
	// uploaded
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_UPLOADED);
	width = setting->download_column.width.uploaded;
	if (width > 0)
		gtk_tree_view_column_set_fixed_width (column, width);
	// ratio
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_RATIO);
	width = setting->download_column.width.ratio;
	if (width > 0)
		gtk_tree_view_column_set_fixed_width (column, width);
	// retry
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_RETRY);
	width = setting->download_column.width.retry;
	if (width > 0)
		gtk_tree_view_column_set_fixed_width (column, width);
	// category
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_CATEGORY);
	width = setting->download_column.width.category;
	if (width > 0)
		gtk_tree_view_column_set_fixed_width (column, width);
	// uri
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_URI);
	width = setting->download_column.width.uri;
	if (width > 0)
		gtk_tree_view_column_set_fixed_width (column, width);
	// added_on
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_ADDED_ON);
	width = setting->download_column.width.added_on;
	if (width > 0)
		gtk_tree_view_column_set_fixed_width (column, width);
	// completed_on
	column = gtk_tree_view_get_column (app->traveler.download.view,
			UGTK_NODE_COLUMN_COMPLETED_ON);
	width = setting->download_column.width.completed_on;
	if (width > 0)
		gtk_tree_view_column_set_fixed_width (column, width);
}

void  ugtk_app_set_plugin_setting (UgtkApp* app, UgtkSetting* setting)
{
	const UgetPluginInfo*  default_plugin;
	int  limit[2];
	int  sensitive = FALSE;

	switch (setting->plugin_order) {
	default:
	case UGTK_PLUGIN_ORDER_CURL:
		uget_app_remove_plugin ((UgetApp*) app, UgetPluginAria2Info);
		default_plugin = UgetPluginCurlInfo;
		sensitive = FALSE;
		break;

	case UGTK_PLUGIN_ORDER_ARIA2:
		uget_app_remove_plugin ((UgetApp*) app, UgetPluginCurlInfo);
		default_plugin = UgetPluginAria2Info;
		sensitive = TRUE;
		break;

	case UGTK_PLUGIN_ORDER_CURL_ARIA2:
		uget_app_add_plugin ((UgetApp*) app, UgetPluginAria2Info);
		default_plugin = UgetPluginCurlInfo;
		sensitive = TRUE;
		break;

	case UGTK_PLUGIN_ORDER_ARIA2_CURL:
		uget_app_add_plugin ((UgetApp*) app, UgetPluginCurlInfo);
		default_plugin = UgetPluginAria2Info;
		sensitive = TRUE;
		break;
	}

	// set default plug-in
	uget_app_set_default_plugin ((UgetApp*) app, default_plugin);
	// set agent plug-in (used by media and MEGA plug-in)
	uget_plugin_agent_global_set(UGET_PLUGIN_AGENT_GLOBAL_PLUGIN,
	                 (void*) default_plugin);
	// set media plug-in
	uget_app_add_plugin ((UgetApp*) app, UgetPluginMediaInfo);
	uget_plugin_global_set(UgetPluginMediaInfo, UGET_PLUGIN_MEDIA_GLOBAL_MATCH_MODE,
	                 (void*)(intptr_t) setting->media.match_mode);
	uget_plugin_global_set(UgetPluginMediaInfo, UGET_PLUGIN_MEDIA_GLOBAL_QUALITY,
	                 (void*)(intptr_t) setting->media.quality);
	uget_plugin_global_set(UgetPluginMediaInfo, UGET_PLUGIN_MEDIA_GLOBAL_TYPE,
	                 (void*)(intptr_t) setting->media.type);
	// set MEGA plug-in
#ifdef HAVE_MEGA
        uget_app_add_plugin ((UgetApp*) app, UgetPluginMegaInfo);
#endif
	// set aria2 plug-in
	if (setting->plugin_order >= UGTK_PLUGIN_ORDER_ARIA2) {
		uget_plugin_global_set(UgetPluginAria2Info, UGET_PLUGIN_ARIA2_GLOBAL_URI,
		                 setting->aria2.uri);
		uget_plugin_global_set(UgetPluginAria2Info, UGET_PLUGIN_ARIA2_GLOBAL_PATH,
		                 setting->aria2.path);
		uget_plugin_global_set(UgetPluginAria2Info, UGET_PLUGIN_ARIA2_GLOBAL_ARGUMENT,
		                 setting->aria2.args);
		uget_plugin_global_set(UgetPluginAria2Info, UGET_PLUGIN_ARIA2_GLOBAL_TOKEN,
		                 setting->aria2.token);
		uget_plugin_global_set(UgetPluginAria2Info, UGET_PLUGIN_ARIA2_GLOBAL_LAUNCH,
		                 (void*)(intptr_t) setting->aria2.launch);
		uget_plugin_global_set(UgetPluginAria2Info, UGET_PLUGIN_ARIA2_GLOBAL_SHUTDOWN,
		                 (void*)(intptr_t) setting->aria2.shutdown);
		limit[0] = setting->aria2.limit.download * 1024;
		limit[1] = setting->aria2.limit.upload * 1024;
		uget_plugin_global_set(UgetPluginAria2Info, UGET_PLUGIN_GLOBAL_SPEED_LIMIT, limit);
	}

//	app->aria2.remote_updated = FALSE;
	gtk_widget_set_sensitive ((GtkWidget*) app->trayicon.menu.create_torrent, sensitive);
	gtk_widget_set_sensitive ((GtkWidget*) app->trayicon.menu.create_metalink, sensitive);
	gtk_widget_set_sensitive ((GtkWidget*) app->menubar.file.create_torrent, sensitive);
	gtk_widget_set_sensitive ((GtkWidget*) app->menubar.file.create_metalink, sensitive);
	gtk_widget_set_sensitive ((GtkWidget*) app->toolbar.create_torrent, sensitive);
	gtk_widget_set_sensitive ((GtkWidget*) app->toolbar.create_metalink, sensitive);
}

void  ugtk_app_set_other_setting (UgtkApp* app, UgtkSetting* setting)
{
	// clipboard & commandline
	ugtk_clipboard_set_pattern (&app->clipboard, setting->clipboard.pattern);
	app->clipboard.website = app->setting.clipboard.website;
	// global speed limit
	uget_task_set_speed (&app->task,
			setting->bandwidth.normal.download * 1024,
			setting->bandwidth.normal.upload   * 1024);
}

void  ugtk_app_set_menu_setting (UgtkApp* app, UgtkSetting* setting)
{
	// ----------------------------------------------------
	// UgtkEditMenu
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.edit.clipboard_monitor,
			setting->clipboard.monitor);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.edit.clipboard_quiet,
			setting->clipboard.quiet);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.edit.commandline_quiet,
			setting->commandline.quiet);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.edit.skip_existing,
			setting->ui.skip_existing);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.edit.apply_recent,
			setting->ui.apply_recent);

	switch (app->setting.completion.action) {
	default:
	case 0:
		gtk_check_menu_item_set_active (
				(GtkCheckMenuItem*)app->menubar.edit.completion.disable, TRUE);
		break;

	case 1:
		gtk_check_menu_item_set_active (
				(GtkCheckMenuItem*)app->menubar.edit.completion.hibernate, TRUE);
		break;

	case 2:
		gtk_check_menu_item_set_active (
				(GtkCheckMenuItem*)app->menubar.edit.completion.suspend, TRUE);
		break;

	case 3:
		gtk_check_menu_item_set_active (
				(GtkCheckMenuItem*)app->menubar.edit.completion.shutdown, TRUE);
		break;

	case 4:
		gtk_check_menu_item_set_active (
				(GtkCheckMenuItem*)app->menubar.edit.completion.reboot, TRUE);
		break;

	case 5:
		gtk_check_menu_item_set_active (
				(GtkCheckMenuItem*)app->menubar.edit.completion.custom, TRUE);
		break;
	}
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.edit.completion.remember,
			setting->completion.remember);
	// ----------------------------------------------------
	// UgtkViewMenu
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.toolbar,
			setting->window.toolbar);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.statusbar,
			setting->window.statusbar);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.category,
			setting->window.category);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.summary,
			setting->window.summary);
	// summary items
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.summary_items.name,
			setting->summary.name);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.summary_items.folder,
			setting->summary.folder);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.summary_items.category,
			setting->summary.category);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.summary_items.uri,
			setting->summary.uri);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.summary_items.message,
			setting->summary.message);
	// download column
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.columns.complete,
			setting->download_column.complete);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.columns.total,
			setting->download_column.total);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.columns.percent,
			setting->download_column.percent);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.columns.elapsed,
			setting->download_column.elapsed);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.columns.left,
			setting->download_column.left);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.columns.speed,
			setting->download_column.speed);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.columns.upload_speed,
			setting->download_column.upload_speed);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.columns.uploaded,
			setting->download_column.uploaded);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.columns.ratio,
			setting->download_column.ratio);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.columns.retry,
			setting->download_column.retry);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.columns.category,
			setting->download_column.category);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.columns.uri,
			setting->download_column.uri);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.columns.added_on,
			setting->download_column.added_on);
	gtk_check_menu_item_set_active (
			(GtkCheckMenuItem*) app->menubar.view.columns.completed_on,
			setting->download_column.completed_on);
}

void  ugtk_app_set_ui_setting (UgtkApp* app, UgtkSetting* setting)
{
	GtkIconSize   icon_size;

#ifdef HAVE_APP_INDICATOR
	// AppIndicator
	ugtk_tray_icon_use_indicator (&app->trayicon, setting->ui.app_indicator);
#endif

	ugtk_tray_icon_set_visible (&app->trayicon, setting->ui.show_trayicon);

	// ----------------------------------------------------
	// large icon
	if (setting->ui.large_icon)
		icon_size = GTK_ICON_SIZE_LARGE_TOOLBAR;
	else
		icon_size = GTK_ICON_SIZE_SMALL_TOOLBAR;
	// toolbar
	gtk_toolbar_set_icon_size ((GtkToolbar*) app->toolbar.self, icon_size);
	// state list
	ugtk_node_view_use_large_icon (app->traveler.state.view,
	                               setting->ui.large_icon,
	                               setting->download_column.width.state);
	// category list
	ugtk_node_view_use_large_icon (app->traveler.category.view,
	                               setting->ui.large_icon,
	                               setting->download_column.width.state);
	// download list
	ugtk_node_view_use_large_icon (app->traveler.download.view,
	                               setting->ui.large_icon,
	                               setting->download_column.width.state);
	// summary
	ugtk_node_view_use_large_icon (app->summary.view,
	                               setting->ui.large_icon,
	                               setting->download_column.width.state);
}

// decide sensitive for menu, toolbar
void  ugtk_app_decide_download_sensitive (UgtkApp* app)
{
	GtkTreeSelection*  selection;
	gboolean           sensitive;
	static gboolean    sensitive_last = TRUE;
	gint               n_selected;

	selection = gtk_tree_view_get_selection (app->traveler.download.view);
	n_selected = gtk_tree_selection_count_selected_rows (selection);
	if (n_selected > 0)
		sensitive = TRUE;
	else
		sensitive = FALSE;

	// change sensitive after select/unselect
	if (sensitive_last != sensitive) {
		sensitive_last  = sensitive;
		gtk_widget_set_sensitive (app->toolbar.runnable, sensitive);
		gtk_widget_set_sensitive (app->toolbar.pause, sensitive);
		gtk_widget_set_sensitive (app->toolbar.properties, sensitive);
		gtk_widget_set_sensitive (app->menubar.download.open, sensitive);
		gtk_widget_set_sensitive (app->menubar.download.open_folder, sensitive);
		gtk_widget_set_sensitive (app->menubar.download.delete, sensitive);
		gtk_widget_set_sensitive (app->menubar.download.delete_file, sensitive);
		gtk_widget_set_sensitive (app->menubar.download.force_start, sensitive);
		gtk_widget_set_sensitive (app->menubar.download.runnable, sensitive);
		gtk_widget_set_sensitive (app->menubar.download.pause, sensitive);
		gtk_widget_set_sensitive (app->menubar.download.move_to.item, sensitive);
		gtk_widget_set_sensitive (app->menubar.download.prioriy.item, sensitive);
	}
	// properties
	if (n_selected > 1) {
		gtk_widget_set_sensitive (app->toolbar.properties, FALSE);
		gtk_widget_set_sensitive (app->menubar.download.properties, FALSE);
	}
	else {
		gtk_widget_set_sensitive (app->toolbar.properties, sensitive);
		gtk_widget_set_sensitive (app->menubar.download.properties, sensitive);
	}

	// Move Up/Down/Top/Bottom functions need reset sensitive when selection changed.
	// These need by  on_move_download_xxx()  series.
	if (n_selected > 0) {
		// Any Category/Status can't move download position if they were sorted.
		if (app->setting.download_column.sort.nth == UGTK_NODE_COLUMN_STATE)
			sensitive = TRUE;
		else
			sensitive = FALSE;
	}
	// move up/down/top/bottom
	gtk_widget_set_sensitive (app->toolbar.move_up, sensitive);
	gtk_widget_set_sensitive (app->toolbar.move_down, sensitive);
	gtk_widget_set_sensitive (app->toolbar.move_top, sensitive);
	gtk_widget_set_sensitive (app->toolbar.move_bottom, sensitive);
	gtk_widget_set_sensitive (app->menubar.download.move_up, sensitive);
	gtk_widget_set_sensitive (app->menubar.download.move_down, sensitive);
	gtk_widget_set_sensitive (app->menubar.download.move_top, sensitive);
	gtk_widget_set_sensitive (app->menubar.download.move_bottom, sensitive);
}

// decide sensitive for menu, toolbar
void  ugtk_app_decide_category_sensitive (UgtkApp* app)
{
	static gboolean  sensitive_last = TRUE;
	gboolean         sensitive;

	if (app->traveler.category.cursor.node)
		sensitive = TRUE;
	else
		sensitive = FALSE;

	if (sensitive_last != sensitive) {
		sensitive_last  = sensitive;
		gtk_widget_set_sensitive (app->menubar.category.properties, sensitive);
		gtk_widget_set_sensitive (app->menubar.view.columns.self, sensitive);
	}
	// cursor at "All Category"
	if (app->traveler.category.cursor.pos == 0) {
		gtk_widget_set_sensitive (app->menubar.file.save_category, FALSE);
		gtk_widget_set_sensitive (app->menubar.category.delete, FALSE);
	}
	else {
		gtk_widget_set_sensitive (app->menubar.file.save_category, sensitive);
		gtk_widget_set_sensitive (app->menubar.category.delete, sensitive);
	}
	// Move Up
	if (app->traveler.category.cursor.pos <= 1)
		gtk_widget_set_sensitive (app->menubar.category.move_up, FALSE);
	else
		gtk_widget_set_sensitive (app->menubar.category.move_up, TRUE);
	// Move Down
	if (app->traveler.category.cursor.pos == 0 ||
	    app->traveler.category.cursor.node->next == NULL)
	{
		gtk_widget_set_sensitive (app->menubar.category.move_down, FALSE);
	}
	else
		gtk_widget_set_sensitive (app->menubar.category.move_down, TRUE);

	ugtk_app_decide_download_sensitive (app);
}

void  ugtk_app_decide_trayicon_visible (UgtkApp* app)
{
	gboolean  visible;

	if (app->setting.ui.show_trayicon)
		visible = TRUE;
	else {
		if (gtk_widget_get_visible ((GtkWidget*) app->window.self))
			visible = FALSE;
		else
			visible = TRUE;
	}
	ugtk_tray_icon_set_visible (&app->trayicon, visible);
}

void  ugtk_app_decide_to_quit (UgtkApp* app)
{
	UgtkConfirmDialog*  cdialog;

	if (app->setting.ui.exit_confirmation == FALSE)
		ugtk_app_quit (app);
	else if (app->dialogs.exit_confirmation)
		gtk_widget_show (app->dialogs.exit_confirmation);
	else {
		cdialog = ugtk_confirm_dialog_new (UGTK_CONFIRM_DIALOG_EXIT, app);
		ugtk_confirm_dialog_run (cdialog);
	}
}

// ------------------------------------
// create node by UI

void  ugtk_app_create_category (UgtkApp* app)
{
	UgtkNodeDialog*  ndialog;
	UgetCommon* common_src;
	UgetCommon* common;
	UgetNode*  cnode_src;
	UgetNode*  cnode;
	gchar*     title;

	title = g_strconcat (UGTK_APP_NAME, " - ", _("New Category"), NULL);
	ndialog = ugtk_node_dialog_new (title, app, TRUE);
	g_free (title);
	ugtk_download_form_set_folders (&ndialog->download, &app->setting);

	// category list
	cnode_src = app->traveler.category.cursor.node->base;
	if (cnode_src->parent != &app->real)
		cnode_src = app->real.children;
	common_src = ug_info_get(cnode_src->info, UgetCommonInfo);
	cnode = uget_node_new (NULL);
	common = ug_info_realloc(cnode->info, UgetCommonInfo);
	ug_info_assign (cnode->info, cnode_src->info, NULL);
	ug_free(common->name);
	common->name = ug_strdup_printf("%s%s", _("Copy - "),
						(common_src->name) ? common_src->name : NULL);

	ugtk_node_dialog_set (ndialog, cnode->info);
	ugtk_node_dialog_run (ndialog, UGTK_NODE_DIALOG_NEW_CATEGORY, cnode);
}

void  ugtk_app_create_download (UgtkApp* app, const char* sub_title, const char* uri)
{
	UgtkNodeDialog*  ndialog;
	UgUri      uuri;
	UgetNode*  cnode;
	GList*     list;
	union {
		gchar*     title;
		UgetNode*  cnode;
	} temp;

	if (sub_title)
		temp.title = g_strconcat (UGTK_APP_NAME, " - ", sub_title, NULL);
	else
		temp.title = g_strconcat (UGTK_APP_NAME, " - ", _("New Download"), NULL);
	ndialog = ugtk_node_dialog_new (temp.title, app, FALSE);
	g_free (temp.title);
	ugtk_download_form_set_folders (&ndialog->download, &app->setting);

	// category list
	cnode = app->traveler.category.cursor.node->base;
	if (cnode->parent != &app->real)
		cnode = app->real.children;

	if (uri != NULL) {
		// set URI entry
		gtk_entry_set_text ((GtkEntry*) ndialog->download.uri_entry, uri);
		ndialog->download.changed.uri = TRUE;
		// match category by URI
		ug_uri_init (&uuri, uri);
		temp.cnode = uget_app_match_category ((UgetApp*) app, &uuri, NULL);
		if (temp.cnode)
			cnode = temp.cnode;
	}
	else if ( (list = ugtk_clipboard_get_uris (&app->clipboard)) != NULL ) {
		// use first URI from clipboard to set URI entry
		gtk_entry_set_text ((GtkEntry*) ndialog->download.uri_entry, list->data);
		ndialog->download.changed.uri = TRUE;
		// match category by URI from clipboard
		ug_uri_init (&uuri, list->data);
		temp.cnode = uget_app_match_category ((UgetApp*) app, &uuri, NULL);
		if (temp.cnode)
			cnode = temp.cnode;
		//
		g_list_free_full (list, g_free);
		ugtk_download_form_complete_entry (&ndialog->download);
	}

	if (cnode)
		cnode = cnode->base;
	ugtk_node_dialog_set_category (ndialog, cnode);
	ugtk_node_dialog_run (ndialog, UGTK_NODE_DIALOG_NEW_DOWNLOAD, NULL);
}

// ------------------------------------
// delete selected node

void  ugtk_app_delete_category (UgtkApp* app)
{
	UgetNode*    cnode;
	int          pos;

	cnode = app->traveler.category.cursor.node->base;
	pos   = app->traveler.category.cursor.pos;
	// move cursor
	if (pos <= 0)
		return;

	if (cnode->next)
		ugtk_traveler_select_category (&app->traveler, pos + 1, -1);
	else {
		if (app->real.n_children > 1)
			ugtk_traveler_select_category (&app->traveler, pos - 1, -1);
		else {
			// The last category will be deleted.
			ugtk_app_add_default_category (app);
			ugtk_traveler_select_category (&app->traveler, pos + 1, -1);
		}
	}
	// remove category
	uget_app_delete_category ((UgetApp*) app, cnode);
	// sync UgtkMenubar.download.move_to
	ugtk_menubar_sync_category (&app->menubar, app, TRUE);
}

void  ugtk_app_delete_download (UgtkApp* app, gboolean delete_files)
{
	UgetNode* cursor;
	UgetNode* node;
	GList*    link;
	GList*    list = NULL;
	// check shift key status
	GdkWindow*       gdk_win;
	GdkDevice*       dev_pointer;
	GdkModifierType  mask;

	// check shift key status
	gdk_win = gtk_widget_get_parent_window ((GtkWidget*) app->traveler.download.view);
	dev_pointer = gdk_device_manager_get_client_pointer (
			gdk_display_get_device_manager (gdk_window_get_display (gdk_win)));
	gdk_window_get_device_position (gdk_win, dev_pointer, NULL, NULL, &mask);

	cursor = app->traveler.download.cursor.node;
	if (cursor)
		cursor = cursor->base;
	list = ugtk_traveler_get_selected (&app->traveler);
	for (link = list;  link;  link = link->next) {
		node = link->data;
		node = node->base;
		link->data = node;
		if (delete_files || mask & GDK_SHIFT_MASK)
			uget_app_delete_download ((UgetApp*) app, node, delete_files);
		else {
			if (uget_app_recycle_download ((UgetApp*) app, node))
				continue;
		}
		// if current node has been deleted
		if (cursor == node)
			cursor = NULL;
		link->data = NULL;
	}
	if (delete_files == FALSE && (mask & GDK_SHIFT_MASK) == 0) {
		ugtk_traveler_set_cursor (&app->traveler, cursor);
		ugtk_traveler_set_selected (&app->traveler, list);
	}
	g_list_free (list);
	// refresh
	gtk_widget_queue_draw ((GtkWidget*) app->traveler.category.view);
	gtk_widget_queue_draw ((GtkWidget*) app->traveler.state.view);

	app->user_action = TRUE;
}

// ------------------------------------
// edit selected node

void  ugtk_app_edit_category (UgtkApp* app)
{
	UgtkNodeDialog* ndialog;
	UgetNode*       node;
	gchar*          title;

	if (app->traveler.category.cursor.pos == 0)
		node = app->real.children;
	else
		node = app->traveler.category.cursor.node;
	if (node == NULL)
		return;

	title = g_strconcat (UGTK_APP_NAME " - ", _("Category Properties"), NULL);
	ndialog = ugtk_node_dialog_new (title, app, TRUE);
	g_free (title);
	ugtk_download_form_set_folders (&ndialog->download, &app->setting);
	ugtk_node_dialog_set (ndialog, node->base->info);
	ugtk_node_dialog_run (ndialog, UGTK_NODE_DIALOG_EDIT_CATEGORY, node->base);
}

void  ugtk_app_edit_download (UgtkApp* app)
{
	UgtkNodeDialog* ndialog;
	UgetNode*       node;
	gchar*          title;

	title = g_strconcat (UGTK_APP_NAME " - ", _("Download Properties"), NULL);
	ndialog = ugtk_node_dialog_new (title, app, FALSE);
	g_free (title);
	ugtk_download_form_set_folders (&ndialog->download, &app->setting);

	node = app->traveler.download.cursor.node;
	ugtk_node_dialog_set (ndialog, node->base->info);
	ugtk_node_dialog_run (ndialog, UGTK_NODE_DIALOG_EDIT_DOWNLOAD, node->base);
}

// ------------------------------------
// queue/pause

void  ugtk_app_queue_download (UgtkApp* app, gboolean keep_active)
{
	UgetRelation* relation;
	UgetNode*  node;
	UgetNode*  cursor;
	GList*     list;
	GList*     link;

	cursor = app->traveler.download.cursor.node;
	if (cursor)
		cursor = cursor->base;

	list = ugtk_traveler_get_selected (&app->traveler);
	for (link = list;  link;  link = link->next) {
		node = link->data;
		node = node->base;
		link->data = node;
		relation = ug_info_realloc(node->info, UgetRelationInfo);
		if (keep_active && relation->group & UGET_GROUP_ACTIVE)
			continue;
		uget_app_queue_download ((UgetApp*) app, node);
	}
	if (app->traveler.state.cursor.pos == 0) {
		ugtk_traveler_set_cursor (&app->traveler, cursor);
		ugtk_traveler_set_selected (&app->traveler, list);
	}
	g_list_free (list);
	// refresh other data & status
	gtk_widget_queue_draw ((GtkWidget*) app->traveler.download.view);
	gtk_widget_queue_draw ((GtkWidget*) app->traveler.category.view);
	gtk_widget_queue_draw ((GtkWidget*) app->traveler.state.view);
//	ugtk_summary_show (&app->summary, app->traveler.download.cursor.node);
}

void  ugtk_app_pause_download (UgtkApp* app)
{
	UgetNode*  node;
	UgetNode*  cursor;
	GList*     list;
	GList*     link;

	cursor = app->traveler.download.cursor.node;
	if (cursor)
		cursor = cursor->base;

	list = ugtk_traveler_get_selected (&app->traveler);
	for (link = list;  link;  link = link->next) {
		node = link->data;
		node = node->base;
		link->data = node;
		uget_app_pause_download ((UgetApp*) app, node);
	}
	if (app->traveler.state.cursor.pos == 0) {
		ugtk_traveler_set_cursor (&app->traveler, cursor);
		ugtk_traveler_set_selected (&app->traveler, list);
	}
	g_list_free (list);
	// refresh other data & status
	gtk_widget_queue_draw ((GtkWidget*) app->traveler.download.view);
	gtk_widget_queue_draw ((GtkWidget*) app->traveler.category.view);
	gtk_widget_queue_draw ((GtkWidget*) app->traveler.state.view);
//	ugtk_summary_show (&app->summary, app->traveler.download.cursor.node);

	app->user_action = TRUE;
}

void  ugtk_app_switch_download_state (UgtkApp* app)
{
	UgetRelation* relation;
	UgetNode*  node;
	UgetNode*  cursor;
	GList*     list;
	GList*     link;

	cursor = app->traveler.download.cursor.node;
	if (cursor)
		cursor = cursor->base;

	list = ugtk_traveler_get_selected (&app->traveler);
	for (link = list;  link;  link = link->next) {
		node = link->data;
		node = node->base;
		link->data = node;
		relation = ug_info_realloc(node->info, UgetRelationInfo);
		if (relation->group & UGET_GROUP_PAUSED)
			uget_app_queue_download ((UgetApp*) app, node);
		else if (relation->group & UGET_GROUP_ACTIVE)
			uget_app_pause_download ((UgetApp*) app, node);
	}
	if (app->traveler.state.cursor.pos == 0) {
		ugtk_traveler_set_cursor (&app->traveler, cursor);
		ugtk_traveler_set_selected (&app->traveler, list);
	}
	g_list_free (list);
	// refresh other data & status
	gtk_widget_queue_draw ((GtkWidget*) app->traveler.download.view);
	gtk_widget_queue_draw ((GtkWidget*) app->traveler.category.view);
	gtk_widget_queue_draw ((GtkWidget*) app->traveler.state.view);
//	ugtk_summary_show (&app->summary, app->traveler.download.cursor.node);

	app->user_action = TRUE;
}

// ------------------------------------
// move selected node

void  ugtk_app_move_download_up (UgtkApp* app)
{
	if (ugtk_traveler_move_selected_up (&app->traveler) > 0) {
		gtk_widget_set_sensitive (app->toolbar.move_down, TRUE);
		gtk_widget_set_sensitive (app->toolbar.move_bottom, TRUE);
		gtk_widget_set_sensitive (app->menubar.download.move_down, TRUE);
		gtk_widget_set_sensitive (app->menubar.download.move_bottom, TRUE);
	}
	else {
		gtk_widget_set_sensitive (app->toolbar.move_up, FALSE);
		gtk_widget_set_sensitive (app->toolbar.move_top, FALSE);
		gtk_widget_set_sensitive (app->menubar.download.move_up, FALSE);
		gtk_widget_set_sensitive (app->menubar.download.move_top, FALSE);
	}
}

void  ugtk_app_move_download_down (UgtkApp* app)
{
	if (ugtk_traveler_move_selected_down (&app->traveler) > 0) {
		gtk_widget_set_sensitive (app->toolbar.move_up, TRUE);
		gtk_widget_set_sensitive (app->toolbar.move_top, TRUE);
		gtk_widget_set_sensitive (app->menubar.download.move_up, TRUE);
		gtk_widget_set_sensitive (app->menubar.download.move_top, TRUE);
	}
	else {
		gtk_widget_set_sensitive (app->toolbar.move_down, FALSE);
		gtk_widget_set_sensitive (app->toolbar.move_bottom, FALSE);
		gtk_widget_set_sensitive (app->menubar.download.move_down, FALSE);
		gtk_widget_set_sensitive (app->menubar.download.move_bottom, FALSE);
	}
}

void  ugtk_app_move_download_top (UgtkApp* app)
{
	if (ugtk_traveler_move_selected_top (&app->traveler) > 0) {
		gtk_widget_set_sensitive (app->toolbar.move_down, TRUE);
		gtk_widget_set_sensitive (app->toolbar.move_bottom, TRUE);
		gtk_widget_set_sensitive (app->menubar.download.move_down, TRUE);
		gtk_widget_set_sensitive (app->menubar.download.move_bottom, TRUE);
	}
	gtk_widget_set_sensitive (app->toolbar.move_up, FALSE);
	gtk_widget_set_sensitive (app->toolbar.move_top, FALSE);
	gtk_widget_set_sensitive (app->menubar.download.move_up, FALSE);
	gtk_widget_set_sensitive (app->menubar.download.move_top, FALSE);
}

void  ugtk_app_move_download_bottom (UgtkApp* app)
{
	if (ugtk_traveler_move_selected_bottom (&app->traveler) > 0) {
		gtk_widget_set_sensitive (app->toolbar.move_up, TRUE);
		gtk_widget_set_sensitive (app->toolbar.move_top, TRUE);
		gtk_widget_set_sensitive (app->menubar.download.move_up, TRUE);
		gtk_widget_set_sensitive (app->menubar.download.move_top, TRUE);
	}
	gtk_widget_set_sensitive (app->toolbar.move_down, FALSE);
	gtk_widget_set_sensitive (app->toolbar.move_bottom, FALSE);
	gtk_widget_set_sensitive (app->menubar.download.move_down, FALSE);
	gtk_widget_set_sensitive (app->menubar.download.move_bottom, FALSE);
}

void  ugtk_app_move_download_to (UgtkApp* app, UgetNode* cnode)
{
	UgetNode* node;
	GList*    link;
	GList*    list = NULL;

	if (cnode == app->traveler.category.cursor.node->base)
		return;

	list = ugtk_traveler_get_selected (&app->traveler);
	for (link = list;  link;  link = link->next) {
		node = link->data;
		node = node->base;
		if (node->parent == cnode)
			continue;
		uget_node_remove (node->parent, node);
		uget_node_clear_fake (node);
		uget_node_append (cnode, node);
	}
	g_list_free (list);
	// refresh
	gtk_widget_queue_draw ((GtkWidget*) app->traveler.category.view);
	gtk_widget_queue_draw ((GtkWidget*) app->traveler.state.view);
}

// ------------------------------------
// torrent & metalink

static GtkWidget*  create_file_chooser (GtkWindow* parent,
                                        GtkFileChooserAction action,
                                        const gchar* title,
                                        const gchar* filter_name,
                                        const gchar* mine_type)
{
	GtkWidget*      dialog;
	GtkFileFilter*  filter;

	dialog = gtk_file_chooser_dialog_new (title,
			parent,
			action,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK,     GTK_RESPONSE_OK,
			NULL);
	gtk_window_set_destroy_with_parent ((GtkWindow*) dialog, TRUE);

	if (filter_name) {
		filter = gtk_file_filter_new ();
		gtk_file_filter_set_name (filter, filter_name);
		gtk_file_filter_add_mime_type (filter, mine_type);
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
	}
	return dialog;
}

static void  on_create_torrent_response (GtkWidget* dialog, gint response, UgtkApp* app)
{
	gchar*  file;

	if (response != GTK_RESPONSE_OK ) {
		gtk_widget_destroy (dialog);
		return;
	}
	// get filename
	file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
//	file = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
	gtk_widget_destroy (dialog);
	ugtk_app_create_download (app, _("New Torrent"), file);
	g_free (file);
}

static void  on_create_metalink_response (GtkWidget* dialog, gint response, UgtkApp* app)
{
	gchar*  file;

	if (response != GTK_RESPONSE_OK) {
		gtk_widget_destroy (dialog);
		return;
	}
	// get filename
	file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
//	file = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
	gtk_widget_destroy (dialog);
	ugtk_app_create_download (app, _("New Metalink"), file);
	g_free (file);
}

void  ugtk_app_create_torrent (UgtkApp* app)
{
	GtkWidget*  dialog;
	gchar*      title;

	title = g_strconcat (UGTK_APP_NAME " - ", _("Open Torrent file"), NULL);
	dialog = create_file_chooser (app->window.self,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			title, _("Torrent file (*.torrent)"), "application/x-bittorrent");
	g_free (title);
	g_signal_connect (dialog, "response",
			G_CALLBACK (on_create_torrent_response), app);
	gtk_widget_show (dialog);
}

void  ugtk_app_create_metalink (UgtkApp* app)
{
	GtkFileFilter*  filter;
	GtkWidget*      dialog;
	gchar*          title;

	title = g_strconcat (UGTK_APP_NAME " - ", _("Open Metalink file"), NULL);
	dialog = create_file_chooser (app->window.self,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			title, NULL, NULL);
	g_free (title);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, "Metalink file (*.metalink, *.meta4)");
	gtk_file_filter_add_pattern (filter, "*.metalink");
	gtk_file_filter_add_pattern (filter, "*.meta4");
//	gtk_file_filter_add_mime_type (filter, "application/metalink+xml");
//	gtk_file_filter_add_mime_type (filter, "application/metalink4+xml");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
	g_signal_connect (dialog, "response",
			G_CALLBACK (on_create_metalink_response), app);
	gtk_widget_show (dialog);
}

// ------------------------------------
// import/export

static void  on_save_category_response (GtkWidget* dialog, gint response, UgtkApp* app)
{
	UgetNode* cnode;
	gchar*    file;

	gtk_widget_set_sensitive ((GtkWidget*) app->window.self, TRUE);
	if (response != GTK_RESPONSE_OK) {
		gtk_widget_destroy (dialog);
		return;
	}
	if (app->traveler.category.cursor.pos == 0)
		return;

	// get filename
	file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
//	file = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
	gtk_widget_destroy (dialog);
	cnode = app->traveler.category.cursor.node;
	if (uget_app_save_category ((UgetApp*) app, cnode->base, file, NULL) == FALSE)
		ugtk_app_show_message (app, GTK_MESSAGE_ERROR, _("Failed to save category file."));
	g_free (file);
}

static void  on_load_category_response (GtkWidget* dialog, gint response, UgtkApp* app)
{
	gchar*  file;

	gtk_widget_set_sensitive ((GtkWidget*) app->window.self, TRUE);
	if (response != GTK_RESPONSE_OK) {
		gtk_widget_destroy (dialog);
		return;
	}
	// get filename
	file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
//	file = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
	gtk_widget_destroy (dialog);
	if (uget_app_load_category ((UgetApp*) app, file, NULL))
		ugtk_menubar_sync_category (&app->menubar, app, TRUE);
	else
		ugtk_app_show_message (app, GTK_MESSAGE_ERROR, _("Failed to load category file."));
	g_free (file);
}

void  ugtk_app_save_category (UgtkApp* app)
{
	GtkWidget*      dialog;
	gchar*          title;

	gtk_widget_set_sensitive ((GtkWidget*) app->window.self, FALSE);

	title = g_strconcat (UGTK_APP_NAME " - ", _("Save Category file"), NULL);
	dialog = create_file_chooser (app->window.self,
			GTK_FILE_CHOOSER_ACTION_SAVE,
			title, NULL, NULL);
	g_free (title);

	g_signal_connect (dialog, "response",
			G_CALLBACK (on_save_category_response), app);
	gtk_widget_show (dialog);
}

void  ugtk_app_load_category (UgtkApp* app)
{
	GtkWidget*      dialog;
	gchar*          title;

	gtk_widget_set_sensitive ((GtkWidget*) app->window.self, FALSE);

	title = g_strconcat (UGTK_APP_NAME " - ", _("Open Category file"), NULL);
	dialog = create_file_chooser (app->window.self,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			title, _("JSON file (*.json)"), "application/json");
	g_free (title);

	g_signal_connect (dialog, "response",
			G_CALLBACK (on_load_category_response), app);
	gtk_widget_show (dialog);
}

static void  on_import_html_file_response (GtkWidget* dialog, gint response, UgtkApp* app)
{
	UgHtmlFilter*     filter;
	UgHtmlFilterTag*  tag_a;
	UgHtmlFilterTag*  tag_img;
	UgtkBatchDialog*  bdialog;
	UgtkSelectorPage* page;
	UgetNode*  cnode;
	gchar*  string;
	gchar*  file;

	if (response != GTK_RESPONSE_OK ) {
		gtk_widget_destroy (dialog);
		return;
	}
	// read URLs from html file
	string = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	gtk_widget_destroy (dialog);
	file = g_filename_to_utf8 (string, -1, NULL, NULL, NULL);
	g_free (string);
	string = NULL;
	// parse html
	filter = ug_html_filter_new ();
	tag_a = ug_html_filter_tag_new ("A", "HREF");		// <A HREF="Link">
	ug_html_filter_add_tag (filter, tag_a);
	tag_img = ug_html_filter_tag_new ("IMG", "SRC");		// <IMG SRC="Link">
	ug_html_filter_add_tag (filter, tag_img);
	ug_html_filter_parse_file (filter, file);
	g_free (file);
	if (filter->base_href)
		string = g_strdup (filter->base_href);
	ug_html_filter_free (filter);
	// UgtkBatchDialog
	bdialog = ugtk_batch_dialog_new (
			gtk_window_get_title ((GtkWindow*) dialog), app);
	ugtk_download_form_set_folders (&bdialog->download, &app->setting);
	ugtk_batch_dialog_use_selector (bdialog);
	// category
	cnode = app->traveler.category.cursor.node->base;
	if (cnode->parent != &app->real)
		cnode = app->real.children;
	ugtk_batch_dialog_set_category (bdialog, cnode);
	// set <base href>
	if (string) {
		gtk_entry_set_text (bdialog->selector.href_entry, string);
		g_free (string);
	}
	// add link
	page = ugtk_selector_add_page (&bdialog->selector, _("Link <A>"));
	ugtk_selector_page_add_uris (page, (GList*)tag_a->attr_values.head);
	ug_list_clear (&tag_a->attr_values, TRUE);
	ug_html_filter_tag_unref (tag_a);
	// add image
	page = ugtk_selector_add_page (&bdialog->selector, _("Image <IMG>"));
	ugtk_selector_page_add_uris (page, (GList*)tag_img->attr_values.head);
	ug_list_clear (&tag_img->attr_values, TRUE);
	ug_html_filter_tag_unref (tag_img);

	ugtk_batch_dialog_run (bdialog);
}

static void  on_import_text_file_response (GtkWidget* dialog, gint response, UgtkApp* app)
{
	UgtkBatchDialog*   bdialog;
	UgtkSelectorPage*  page;
	UgetNode* cnode;
	gchar*    string;
	gchar*    file;
	GList*    list;
	GError*   error = NULL;

	if (response != GTK_RESPONSE_OK ) {
		gtk_widget_destroy (dialog);
		return;
	}
	// read URLs from text file
	string = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	gtk_widget_destroy (dialog);
	file = g_filename_to_utf8 (string, -1, NULL, NULL, NULL);
	g_free (string);
	list = ugtk_text_file_get_uris (file, &error);
	g_free (file);
	if (error) {
		ugtk_app_show_message (app, GTK_MESSAGE_ERROR, error->message);
		g_error_free (error);
		return;
	}
	// UgtkBatchDialog
	bdialog = ugtk_batch_dialog_new (
			gtk_window_get_title ((GtkWindow*) dialog), app);
	ugtk_batch_dialog_use_selector (bdialog);
	ugtk_download_form_set_folders (&bdialog->download, &app->setting);
	// category
	cnode = app->traveler.category.cursor.node->base;
	if (cnode->parent != &app->real)
		cnode = app->real.children;
	ugtk_batch_dialog_set_category (bdialog, cnode);

	page = ugtk_selector_add_page (&bdialog->selector, _("Text File"));
	ugtk_selector_hide_href (&bdialog->selector);
	ugtk_selector_page_add_uris (page, list);
	g_list_free (list);

	ugtk_batch_dialog_run (bdialog);
}

static void  on_export_text_file_response (GtkWidget* dialog, gint response, UgtkApp* app)
{
	GIOChannel*  channel;
	UgetCommon*  common;
	UgetNode*    node;
	gchar*       fname;

	if (response != GTK_RESPONSE_OK ) {
		gtk_widget_destroy (dialog);
		return;
	}
	// write all URLs to text file
	fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	gtk_widget_destroy (dialog);
	channel = g_io_channel_new_file (fname, "w", NULL);
	g_free (fname);

	node = app->traveler.category.cursor.node->base;
	for (node = node->children;  node;  node = node->next) {
		common = ug_info_get (node->info, UgetCommonInfo);
		if (common == NULL)
			continue;
		if (common->uri) {
			g_io_channel_write_chars (channel, common->uri, -1, NULL, NULL);
#ifdef _WIN32
			g_io_channel_write_chars (channel, "\r\n", 2, NULL, NULL);
#else
			g_io_channel_write_chars (channel, "\n", 1, NULL, NULL);
#endif
		}
	}
	g_io_channel_unref (channel);
}

void  ugtk_app_import_html_file (UgtkApp* app)
{
	GtkWidget*  dialog;
	gchar*      title;

	title = g_strconcat (UGTK_APP_NAME " - ", _("Import URLs from HTML file"), NULL);
	dialog = create_file_chooser (app->window.self,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			title, _("HTML file (*.htm, *.html)"), "text/html");
	g_free (title);
	g_signal_connect (dialog, "response",
			G_CALLBACK (on_import_html_file_response), app);
	gtk_widget_show (dialog);
}

void  ugtk_app_import_text_file (UgtkApp* app)
{
	GtkWidget*  dialog;
	gchar*      title;

	title = g_strconcat (UGTK_APP_NAME " - ", _("Import URLs from text file"), NULL);
	dialog = create_file_chooser (app->window.self,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			title, _("Plain text file"), "text/plain");
	g_free (title);
	g_signal_connect (dialog, "response",
			G_CALLBACK (on_import_text_file_response), app);
	gtk_widget_show (dialog);
}

void  ugtk_app_export_text_file (UgtkApp* app)
{
	GtkWidget*  dialog;
	gchar*      title;

	title = g_strconcat (UGTK_APP_NAME " - ", _("Export URLs to text file"), NULL);
	dialog = create_file_chooser (app->window.self,
			GTK_FILE_CHOOSER_ACTION_SAVE,
			title, NULL, NULL);
	g_free (title);
	g_signal_connect (dialog, "response",
			G_CALLBACK (on_export_text_file_response), app);
	gtk_widget_show (dialog);
}

// ------------------------------------
// batch

void  ugtk_app_sequence_batch (UgtkApp* app)
{
	UgtkBatchDialog* bdialog;
	UgetNode*  cnode;
	gchar*     title;

	title = g_strconcat (UGTK_APP_NAME " - ", _("URL Sequence batch"), NULL);
	bdialog = ugtk_batch_dialog_new (title, app);
	g_free (title);
	ugtk_batch_dialog_use_sequencer (bdialog);
	ugtk_download_form_set_folders (&bdialog->download, &app->setting);

	// category list
	cnode = app->traveler.category.cursor.node->base;
	if (cnode->parent != &app->real)
		cnode = app->real.children;
	ugtk_batch_dialog_set_category (bdialog, cnode);

	ugtk_batch_dialog_run (bdialog);
}

void  ugtk_app_clipboard_batch (UgtkApp* app)
{
	UgtkBatchDialog*  bdialog;
	UgtkSelectorPage* page;
	UgetNode*  cnode;
	GList*     list;
	gchar*     title;

	list = ugtk_clipboard_get_uris (&app->clipboard);
	if (list == NULL) {
		ugtk_app_show_message (app, GTK_MESSAGE_ERROR,
				_("No URLs found in clipboard."));
		return;
	}
	// filter existing
	if (app->setting.ui.skip_existing) {
		if (ugtk_app_filter_existing (app, list) == 0) {
//			g_list_foreach (list, (GFunc) g_free, NULL);
			g_list_free (list);
			ugtk_app_show_message (app, GTK_MESSAGE_INFO,
					_("All URLs had existed."));
			return;
		}
	}

	title = g_strconcat (UGTK_APP_NAME " - ", _("Clipboard batch"), NULL);
	bdialog = ugtk_batch_dialog_new (title, app);
	g_free (title);
	ugtk_download_form_set_folders (&bdialog->download, &app->setting);
	ugtk_batch_dialog_use_selector (bdialog);
	// selector
	ugtk_selector_hide_href (&bdialog->selector);
	page = ugtk_selector_add_page (&bdialog->selector, _("Clipboard"));
	ugtk_selector_page_add_uris (page, list);
	g_list_free (list);

	// category list
	cnode = app->traveler.category.cursor.node->base;
	if (cnode->parent != &app->real)
		cnode = app->real.children;
	ugtk_batch_dialog_set_category (bdialog, cnode);

	ugtk_batch_dialog_run (bdialog);
}

int   ugtk_app_filter_existing (UgtkApp* app, GList* uris)
{
	int  counts;

	for (counts = 0;  uris;  uris = uris->next) {
		if (uris->data == NULL)
			continue;
		if (uget_uri_hash_find (app->uri_hash, (char*) uris->data) == FALSE)
			counts++;
		else {
			g_free (uris->data);
			uris->data = NULL;
		}
	}
	return counts;
}

// ------------------------------------
// emit signal "row-changed" for UgtkNodeTree and UgtkNodeList

void  ugtk_app_download_changed (UgtkApp* app, UgetNode* dnode)
{
	GtkTreeModel* model;
	GtkTreePath*  path;
	GtkTreeIter   iter;

	if (ugtk_traveler_get_iter (&app->traveler, &iter, dnode)) {
		model = GTK_TREE_MODEL (app->traveler.download.model);
		path = gtk_tree_model_get_path (model, &iter);
		if (path) {
			gtk_tree_model_row_changed (model, path, &iter);
			gtk_tree_path_free (path);
		}
	}
}

void  ugtk_app_category_changed (UgtkApp* app, UgetNode* cnode)
{
	GtkTreeIter   iter;
	GtkTreePath*  path;
	GtkTreeModel* model;

	model = GTK_TREE_MODEL (app->traveler.category.model);
	if (app->traveler.category.cursor.pos > 0) {
		iter.stamp = app->traveler.category.model->stamp;
		iter.user_data = cnode;
		// update category
		path = gtk_tree_model_get_path (model, &iter);
		gtk_tree_model_row_changed (model, path, &iter);
		gtk_tree_path_free (path);
	}
	// update "All Category"
	path = gtk_tree_path_new_first ();
	gtk_tree_model_get_iter_first (model, &iter);
	gtk_tree_model_row_changed (model, path, &iter);
	gtk_tree_path_free (path);

	// refresh status list
	gtk_widget_queue_draw ((GtkWidget*) app->traveler.state.view);
}

void  ugtk_app_add_default_category (UgtkApp* app)
{
	UgetNode*     cnode;
	UgetCommon*   common;
	UgetCategory* category;
	static int    counts = 0;

	cnode = uget_node_new (NULL);
	common = ug_info_realloc (cnode->info, UgetCommonInfo);
	common->name = ug_strdup_printf ("%s %d", _("New"), counts++);
	common->folder = ug_strdup (g_get_user_special_dir (G_USER_DIRECTORY_DOWNLOAD));
	category = ug_info_realloc (cnode->info, UgetCategoryInfo);
	*(char**)ug_array_alloc (&category->schemes, 1) = ug_strdup ("ftps");
	*(char**)ug_array_alloc (&category->schemes, 1) = ug_strdup ("magnet");
	*(char**)ug_array_alloc (&category->hosts, 1) = ug_strdup (".edu");
	*(char**)ug_array_alloc (&category->hosts, 1) = ug_strdup (".idv");
	*(char**)ug_array_alloc (&category->file_exts, 1) = ug_strdup ("torrent");
	*(char**)ug_array_alloc (&category->file_exts, 1) = ug_strdup ("metalink");

	uget_app_add_category ((UgetApp*) app, cnode, TRUE);
}

// ------------------------------------
// others

static void  on_message_response (GtkWidget* dialog, gint response, GtkWidget** value)
{
	gtk_widget_destroy (dialog);
	*value = NULL;
}

void  ugtk_app_show_message (UgtkApp* app, GtkMessageType type,
                             const gchar* message)
{
	GtkWidget*		dialog;
	GtkWidget**		value;
	gchar*			title;

	dialog = gtk_message_dialog_new (app->window.self,
			GTK_DIALOG_DESTROY_WITH_PARENT,
			type, GTK_BUTTONS_OK,
			"%s", message);
	// set title
	switch (type) {
	case GTK_MESSAGE_ERROR:
		if (app->dialogs.error)
			gtk_widget_destroy (app->dialogs.error);
		app->dialogs.error = dialog;
		value = &app->dialogs.error;
		title = g_strconcat (UGTK_APP_NAME " - ", _("Error"), NULL);
		break;

	default:
		if (app->dialogs.message)
			gtk_widget_destroy (app->dialogs.message);
		app->dialogs.message = dialog;
		value = &app->dialogs.message;
		title = g_strconcat (UGTK_APP_NAME " - ", _("Message"), NULL);
		break;
	}
	gtk_window_set_title ((GtkWindow*) dialog, title);
	g_free (title);
	// signal handler
	g_signal_connect (dialog, "response",
			G_CALLBACK (on_message_response), value);
	gtk_widget_show (dialog);
}

// -------------------------------------------------------
// UgtkClipboard

void  ugtk_clipboard_init (struct UgtkClipboard* clipboard, const gchar* pattern)
{
	clipboard->self  = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	clipboard->text  = NULL;
	clipboard->regex = g_regex_new (pattern, G_REGEX_CASELESS, 0, NULL);
	clipboard->website = TRUE;
}

void  ugtk_clipboard_set_pattern (struct UgtkClipboard* clipboard, const gchar* pattern)
{
	if (clipboard->regex)
		g_regex_unref (clipboard->regex);

	if (pattern)
		clipboard->regex = g_regex_new (pattern, G_REGEX_CASELESS, 0, NULL);
	else
		clipboard->regex = g_regex_new ("", G_REGEX_CASELESS, 0, NULL);
}

void  ugtk_clipboard_set_text (struct UgtkClipboard* clipboard, gchar* text)
{
	g_free (clipboard->text);
	clipboard->text = text;
	gtk_clipboard_set_text (clipboard->self, text, -1);
}

GList* ugtk_clipboard_get_uris (struct UgtkClipboard* clipboard)
{
	GList*		list;
	gchar*		text;

	if (gtk_clipboard_wait_is_text_available (clipboard->self) == FALSE)
		return NULL;
	text = gtk_clipboard_wait_for_text (clipboard->self);
	if (text == NULL)
		return NULL;
	// get URIs that scheme is not "file" from text
	list = ugtk_text_get_uris (text);
	list = ugtk_uri_list_remove_scheme (list, "file");
	g_free (text);

	return list;
}

GList* ugtk_clipboard_get_matched (struct UgtkClipboard* clipboard, const gchar* text)
{
	GList*		link;
	GList*		list;
	gchar*		temp;

	if (text == NULL) {
		g_free (clipboard->text);
		clipboard->text = NULL;
		return NULL;
	}
	// compare
	temp = (clipboard->text) ? clipboard->text : "";
	if (g_ascii_strcasecmp (text, temp) == 0)
		return NULL;
	// replace text
	g_free (clipboard->text);
	clipboard->text = g_strdup (text);
	// get and filter list
	list = ugtk_text_get_uris (text);
	list = ugtk_uri_list_remove_scheme (list, "file");
	// filter by filename extension
	for (link = list;  link;  link = link->next) {
		temp = ug_filename_from_uri (link->data);
		// get filename extension
		if (temp)
			text = strrchr (temp, '.');
		else
			text = NULL;
		// free URIs if not matched
		if (text == NULL || g_regex_match (clipboard->regex, text+1, 0, NULL) == FALSE) {
			// storage or media website
			if (clipboard->website == FALSE ||
			     uget_site_get_id (link->data) == UGET_SITE_UNKNOWN)
			{
				g_free (link->data);
				link->data = NULL;
			}
		}
		ug_free (temp);
	}
	list = g_list_remove_all (list, NULL);
	return list;
}

// -------------------------------------------------------
// UgtkStatusbar

void  ugtk_statusbar_set_info (struct UgtkStatusbar* statusbar, gint n_selected)
{
	static guint	context_id = 0;
	gchar*			string;

	if (context_id == 0)
		context_id = gtk_statusbar_get_context_id (statusbar->self, "selected");
	gtk_statusbar_pop  (statusbar->self, context_id);

	if (n_selected > 0) {
		string = g_strdup_printf (_("Selected %d items"), n_selected);
		gtk_statusbar_push (statusbar->self, context_id, string);
		g_free (string);
	}
}

void  ugtk_statusbar_set_speed (struct UgtkStatusbar* statusbar, gint64 down_speed, gint64 up_speed)
{
	char*		string;

	string = ug_str_from_int_unit (down_speed, "/s");
	gtk_label_set_text (statusbar->down_speed, string);
	ug_free (string);

	string = ug_str_from_int_unit (up_speed, "/s");
	gtk_label_set_text (statusbar->up_speed, string);
	ug_free (string);
}

