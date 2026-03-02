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

#include <strings.h>
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
#include <UgtkSettingDialog.h>

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
	uget_plugin_global_set(UgetPluginMegaInfo,  UGET_PLUGIN_GLOBAL_INIT, (void*) TRUE);
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
	gtk_widget_set_visible(app->banner.self, FALSE);

	uget_app_use_uri_hash ((UgetApp*) app);
	ugtk_app_init_timeout (app);

	if (app->setting.ui.start_in_tray)
		ugtk_tray_icon_set_visible (&app->trayicon, TRUE);
	else
		gtk_widget_set_visible((GtkWidget*) app->window.self, TRUE);
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
	uget_plugin_global_set(UgetPluginMegaInfo,  UGET_PLUGIN_GLOBAL_INIT, (void*) FALSE);
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
	gtk_widget_set_visible(GTK_WIDGET (app->window.self), FALSE);

	// gtk_main_quit ();
	exit(0);
}

void  ugtk_app_get_window_setting (UgtkApp* app, UgtkSetting* setting)
{
	// GdkWindowState  gdk_wstate;
	// GdkWindow*      gdk_window;
	// gint            x, y;

	// get window position, size, and maximized state
	if (gtk_widget_get_visible (GTK_WIDGET (app->window.self)) == TRUE) {
		// gdk_window = gtk_widget_get_window (GTK_WIDGET (app->window.self));
		// gdk_wstate = gdk_window_get_state (gdk_window);

		if (gtk_window_is_maximized ((GtkWindow*) app->window.self))
			setting->window.maximized = TRUE;
		else
			setting->window.maximized = FALSE;
		// get geometry
		// GTK4: Getting window position is not recommended/supported consistently across backends
		// if (setting->window.maximized == FALSE) {
		// 	gtk_window_get_position (app->window.self, &x, &y);
		// 	gtk_window_get_size (app->window.self,
		// 			&setting->window.width, &setting->window.height);
		// 	// gtk_window_get_position() may return: x == -32000, y == -32000
		// 	if (x + app->setting.window.width > 0)
		// 		setting->window.x = x;
		// 	if (y + app->setting.window.height > 0)
		// 		setting->window.y = y;
		// }
		gtk_window_get_default_size ((GtkWindow*) app->window.self,
				&setting->window.width, &setting->window.height);
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
	// GTK4: Window position setting is deprecated/removed
	// if (setting->window.width  > 0 &&
	//     setting->window.height > 0 &&
	//     setting->window.x < gdk_screen_width ()  &&
	//     setting->window.y < gdk_screen_height () &&
	//     setting->window.x + setting->window.width > 0  &&
	//     setting->window.y + setting->window.height > 0)
	// {
	// 	gtk_window_move (app->window.self,
	// 			setting->window.x, setting->window.y);
	// 	gtk_window_resize (app->window.self,
	// 			setting->window.width, setting->window.height);
	// }
	if (setting->window.width  > 0 && setting->window.height > 0) {
		gtk_window_set_default_size ((GtkWindow*) app->window.self,
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
	gtk_widget_set_visible (gtk_paned_get_start_child (app->window.hpaned),
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

// helper: get GtkColumnViewColumn by index
static GtkColumnViewColumn* get_cv_column (GtkColumnView* view, int nth)
{
	GListModel* columns = gtk_column_view_get_columns (view);
	GtkColumnViewColumn* col = g_list_model_get_item (columns, nth);
	// caller must g_object_unref
	return col;
}

void  ugtk_app_get_column_setting (UgtkApp* app, UgtkSetting* setting)
{
	GtkColumnViewColumn* column;

	// For GtkColumnView, we use fixed_width since there's no dynamic "get_width"
	// that returns the actual rendered width. We just read back what we set.
	column = get_cv_column (app->traveler.download.view, UGTK_NODE_COLUMN_STATE);
	setting->download_column.width.state = gtk_column_view_column_get_fixed_width (column);
	g_object_unref (column);
	column = get_cv_column (app->traveler.download.view, UGTK_NODE_COLUMN_NAME);
	setting->download_column.width.name = gtk_column_view_column_get_fixed_width (column);
	g_object_unref (column);
	column = get_cv_column (app->traveler.download.view, UGTK_NODE_COLUMN_COMPLETE);
	setting->download_column.width.complete = gtk_column_view_column_get_fixed_width (column);
	g_object_unref (column);
	column = get_cv_column (app->traveler.download.view, UGTK_NODE_COLUMN_TOTAL);
	setting->download_column.width.total = gtk_column_view_column_get_fixed_width (column);
	g_object_unref (column);
	column = get_cv_column (app->traveler.download.view, UGTK_NODE_COLUMN_PERCENT);
	setting->download_column.width.percent = gtk_column_view_column_get_fixed_width (column);
	g_object_unref (column);
	column = get_cv_column (app->traveler.download.view, UGTK_NODE_COLUMN_ELAPSED);
	setting->download_column.width.elapsed = gtk_column_view_column_get_fixed_width (column);
	g_object_unref (column);
	column = get_cv_column (app->traveler.download.view, UGTK_NODE_COLUMN_LEFT);
	setting->download_column.width.left = gtk_column_view_column_get_fixed_width (column);
	g_object_unref (column);
	column = get_cv_column (app->traveler.download.view, UGTK_NODE_COLUMN_SPEED);
	setting->download_column.width.speed = gtk_column_view_column_get_fixed_width (column);
	g_object_unref (column);
	column = get_cv_column (app->traveler.download.view, UGTK_NODE_COLUMN_UPLOAD_SPEED);
	setting->download_column.width.upload_speed = gtk_column_view_column_get_fixed_width (column);
	g_object_unref (column);
	column = get_cv_column (app->traveler.download.view, UGTK_NODE_COLUMN_UPLOADED);
	setting->download_column.width.uploaded = gtk_column_view_column_get_fixed_width (column);
	g_object_unref (column);
	column = get_cv_column (app->traveler.download.view, UGTK_NODE_COLUMN_RATIO);
	setting->download_column.width.ratio = gtk_column_view_column_get_fixed_width (column);
	g_object_unref (column);
	column = get_cv_column (app->traveler.download.view, UGTK_NODE_COLUMN_RETRY);
	setting->download_column.width.retry = gtk_column_view_column_get_fixed_width (column);
	g_object_unref (column);
	column = get_cv_column (app->traveler.download.view, UGTK_NODE_COLUMN_CATEGORY);
	setting->download_column.width.category = gtk_column_view_column_get_fixed_width (column);
	g_object_unref (column);
	column = get_cv_column (app->traveler.download.view, UGTK_NODE_COLUMN_URI);
	setting->download_column.width.uri = gtk_column_view_column_get_fixed_width (column);
	g_object_unref (column);
	column = get_cv_column (app->traveler.download.view, UGTK_NODE_COLUMN_ADDED_ON);
	setting->download_column.width.added_on = gtk_column_view_column_get_fixed_width (column);
	g_object_unref (column);
	column = get_cv_column (app->traveler.download.view, UGTK_NODE_COLUMN_COMPLETED_ON);
	setting->download_column.width.completed_on = gtk_column_view_column_get_fixed_width (column);
	g_object_unref (column);
}

static void set_cv_column_width (GtkColumnView* view, int nth, int width)
{
	GtkColumnViewColumn* column;
	if (width <= 0)
		return;
	column = get_cv_column (view, nth);
	gtk_column_view_column_set_fixed_width (column, width);
	g_object_unref (column);
}

void  ugtk_app_set_column_setting (UgtkApp* app, UgtkSetting* setting)
{
	GtkColumnView* view = app->traveler.download.view;

	set_cv_column_width (view, UGTK_NODE_COLUMN_STATE, setting->download_column.width.state);
	set_cv_column_width (view, UGTK_NODE_COLUMN_NAME, setting->download_column.width.name);
	set_cv_column_width (view, UGTK_NODE_COLUMN_COMPLETE, setting->download_column.width.complete);
	set_cv_column_width (view, UGTK_NODE_COLUMN_TOTAL, setting->download_column.width.total);
	set_cv_column_width (view, UGTK_NODE_COLUMN_PERCENT, setting->download_column.width.percent);
	set_cv_column_width (view, UGTK_NODE_COLUMN_ELAPSED, setting->download_column.width.elapsed);
	set_cv_column_width (view, UGTK_NODE_COLUMN_LEFT, setting->download_column.width.left);
	set_cv_column_width (view, UGTK_NODE_COLUMN_SPEED, setting->download_column.width.speed);
	set_cv_column_width (view, UGTK_NODE_COLUMN_UPLOAD_SPEED, setting->download_column.width.upload_speed);
	set_cv_column_width (view, UGTK_NODE_COLUMN_UPLOADED, setting->download_column.width.uploaded);
	set_cv_column_width (view, UGTK_NODE_COLUMN_RATIO, setting->download_column.width.ratio);
	set_cv_column_width (view, UGTK_NODE_COLUMN_RETRY, setting->download_column.width.retry);
	set_cv_column_width (view, UGTK_NODE_COLUMN_CATEGORY, setting->download_column.width.category);
	set_cv_column_width (view, UGTK_NODE_COLUMN_URI, setting->download_column.width.uri);
	set_cv_column_width (view, UGTK_NODE_COLUMN_ADDED_ON, setting->download_column.width.added_on);
	set_cv_column_width (view, UGTK_NODE_COLUMN_COMPLETED_ON, setting->download_column.width.completed_on);
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
	uget_app_add_plugin ((UgetApp*) app, UgetPluginMegaInfo);
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
	ugtk_tray_icon_set_plugin_sensitive (&app->trayicon, sensitive);
	if (app->menubar.file.create_torrent)
		gtk_widget_set_sensitive ((GtkWidget*) app->menubar.file.create_torrent, sensitive);
	if (app->menubar.file.create_metalink)
		gtk_widget_set_sensitive ((GtkWidget*) app->menubar.file.create_metalink, sensitive);
	if (app->toolbar.create_torrent)
		gtk_widget_set_sensitive ((GtkWidget*) app->toolbar.create_torrent, sensitive);
	if (app->toolbar.create_metalink)
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
#if 0
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
#endif
}

void  ugtk_app_set_ui_setting (UgtkApp* app, UgtkSetting* setting)
{
	ugtk_app_decide_trayicon_visible (app);
	ugtk_tray_icon_sync_menu (&app->trayicon, app);

}

// decide sensitive for menu, toolbar
void  ugtk_app_decide_download_sensitive (UgtkApp* app)
{
	gboolean           sensitive;
	static gboolean    sensitive_last = TRUE;
	gint               n_selected;
	GAction*           action;
	GtkBitset*         bitset;

	bitset = gtk_selection_model_get_selection (GTK_SELECTION_MODEL (app->traveler.download.selection));
	n_selected = gtk_bitset_get_size (bitset);
	gtk_bitset_unref (bitset);
	if (n_selected > 0)
		sensitive = TRUE;
	else
		sensitive = FALSE;

	// Update GAction states (GTK4)
	const char *actions[] = {
		"download-open",
		"download-open-folder",
		"download-delete",
		"download-delete-file",
		"download-force-start",
		"download-runnable",	// "download-start" mapped to runnable in menu? need verification, likely "download-start"
		"download-start",
		"download-pause",
		"download-move-up",
		"download-move-down",
		"download-move-top",
		"download-move-bottom",
		"download-properties",
		// "priority", // submenu actions handled via GAction? Usually "priority" is the group or state.
		NULL
	};

	// "runnable" action name check: in menu it was "win.download-start", but callback is on_action_download_start?
	// let's try both common names just in case, or stick to what we saw in UgtkMenubar-ui.c ("win.download-start")
	// The action name in group likely doesn't have "win.". "download-start" is safe.

	for (const char **act = actions; *act; act++) {
		action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), *act);
		if (action) {
			g_simple_action_set_enabled (G_SIMPLE_ACTION (action), sensitive);
		}
	}
	// "download-runnable" might be the name used in some contexts, but if not found it just skips.

	// change sensitive after select/unselect (Toolbar widgets)
	if (sensitive_last != sensitive) {
		sensitive_last  = sensitive;
		gtk_widget_set_sensitive (app->toolbar.runnable, sensitive);
		gtk_widget_set_sensitive (app->toolbar.pause, sensitive);
		gtk_widget_set_sensitive (app->toolbar.properties, sensitive);
		// App menubar widgets (if used)
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
		action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "download-properties");
		if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
	}
	else {
		gtk_widget_set_sensitive (app->toolbar.properties, sensitive);
		gtk_widget_set_sensitive (app->menubar.download.properties, sensitive);
		// properties enabledstate handled by loop above for sensistive=TRUE, but if sensitive=TRUE and n>1 must disable
	}

	// Move Up/Down/Top/Bottom functions need reset sensitive when selection changed.
	// These need by  on_move_download_xxx()  series.
	if (n_selected > 0) {
		// Enabled even if sorted - move action will implicitly switch to manual sort
		sensitive = TRUE;
	} else {
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
	
	// Update action state for move commands too
	const char *move_actions[] = { "download-move-up", "download-move-down", "download-move-top", "download-move-bottom", NULL };
	for (const char **act = move_actions; *act; act++) {
		action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), *act);
		if (action) g_simple_action_set_enabled (G_SIMPLE_ACTION (action), sensitive);
	}
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
	else if (app->setting.ui.close_to_tray || app->setting.ui.start_in_tray)
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
		gtk_window_present ((GtkWindow*) app->dialogs.exit_confirmation);
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

// Async callback for clipboard text
static void on_paste_url_received (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    GdkClipboard *clipboard = GDK_CLIPBOARD (source_object);
    UgtkNodeDialog *ndialog = (UgtkNodeDialog *) user_data;
    char *text;
    GError *error = NULL;

    text = gdk_clipboard_read_text_finish (clipboard, res, &error);
    if (text) {
        GList *list;
        // reuse parsing logic
        list = ugtk_text_get_uris (text);
        list = ugtk_uri_list_remove_scheme (list, "file");
        if (list) {
            // use first URI
            gtk_editable_set_text (GTK_EDITABLE (ndialog->download.uri_entry), (const char*)list->data);
            ndialog->download.changed.uri = TRUE;
            
            // match category by URI
            UgUri uuri;
            UgetNode *temp_cnode;
            ug_uri_init (&uuri, (const char*)list->data);
            temp_cnode = uget_app_match_category ((UgetApp*) ndialog->app, &uuri, NULL);
            if (temp_cnode) {
                 ugtk_node_dialog_set_category (ndialog, temp_cnode->base);
            }
            g_list_free_full (list, g_free);
            ugtk_download_form_complete_entry (&ndialog->download);
        }
        g_free (text);
    } else {
        if (error) g_error_free (error);
    }
}

void  ugtk_app_create_download (UgtkApp* app, const char* sub_title, const char* uri)
{
	UgtkNodeDialog*  ndialog;
	UgUri      uuri;
	UgetNode*  cnode;
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
		gtk_editable_set_text (GTK_EDITABLE (ndialog->download.uri_entry), uri);
		ndialog->download.changed.uri = TRUE;
		// match category by URI
		ug_uri_init (&uuri, uri);
		temp.cnode = uget_app_match_category ((UgetApp*) app, &uuri, NULL);
		if (temp.cnode)
			cnode = temp.cnode;
	}
	else {
        // GTK4 Async Clipboard Read
        GdkClipboard *clipboard = gdk_display_get_clipboard (gdk_display_get_default ());
        if (clipboard) {
            gdk_clipboard_read_text_async (clipboard, NULL, on_paste_url_received, ndialog);
        }
    }
    
#if 0
	else if ( (list = ugtk_clipboard_get_uris (&app->clipboard)) != NULL ) {
		// use first URI from clipboard to set URI entry
		gtk_editable_set_text (GTK_EDITABLE (ndialog->download.uri_entry), list->data);
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
#endif

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
#if 0
	GdkWindow*       gdk_win;
	GdkDeviceManager* device_manager;
	GdkDevice*       dev_pointer;
	GdkModifierType  mask;
	gint             x, y;

	gdk_win = gtk_widget_get_parent_window ((GtkWidget*) app->traveler.download.view);
	dev_pointer = gdk_device_manager_get_client_pointer (
			gdk_display_get_device_manager (gdk_window_get_display (gdk_win)));
	gdk_window_get_device_position (gdk_win, dev_pointer, NULL, NULL, &mask);
#endif

	cursor = app->traveler.download.cursor.node;
	if (cursor)
		cursor = cursor->base;
	list = ugtk_traveler_get_selected (&app->traveler);
	for (link = list;  link;  link = link->next) {
		node = link->data;
		node = node->base;
		link->data = node;
		if (delete_files /* || mask & GDK_SHIFT_MASK */)
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
	if (delete_files == FALSE /* && (mask & GDK_SHIFT_MASK) == 0 */) {
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

static void  on_create_torrent_done (GObject* source, GAsyncResult* result, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	GFile* gfile = gtk_file_dialog_open_finish (GTK_FILE_DIALOG (source), result, NULL);
	gchar* file = NULL;

	if (gfile) {
		file = g_file_get_path (gfile);
		g_object_unref (gfile);
	}
	if (file) {
		ugtk_app_create_download (app, _("New Torrent"), file);
		g_free (file);
	}
}

static void  on_create_metalink_done (GObject* source, GAsyncResult* result, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	GFile* gfile = gtk_file_dialog_open_finish (GTK_FILE_DIALOG (source), result, NULL);
	gchar* file = NULL;

	if (gfile) {
		file = g_file_get_path (gfile);
		g_object_unref (gfile);
	}
	if (file) {
		ugtk_app_create_download (app, _("New Metalink"), file);
		g_free (file);
	}
}

void  ugtk_app_create_torrent (UgtkApp* app)
{
	GtkFileDialog*  dialog;
	GtkFileFilter*  filter;
	GListStore*     filters;
	gchar*          title;

	title = g_strconcat (UGTK_APP_NAME " - ", _("Open Torrent file"), NULL);
	dialog = gtk_file_dialog_new ();
	gtk_file_dialog_set_title (dialog, title);
	g_free (title);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Torrent file (*.torrent)"));
	gtk_file_filter_add_mime_type (filter, "application/x-bittorrent");
	filters = g_list_store_new (GTK_TYPE_FILE_FILTER);
	g_list_store_append (filters, filter);
	g_object_unref (filter);
	gtk_file_dialog_set_filters (dialog, G_LIST_MODEL (filters));
	g_object_unref (filters);

	gtk_file_dialog_open (dialog, app->window.self, NULL,
			on_create_torrent_done, app);
	g_object_unref (dialog);
}

void  ugtk_app_create_metalink (UgtkApp* app)
{
	GtkFileDialog*  dialog;
	GtkFileFilter*  filter;
	GListStore*     filters;
	gchar*          title;

	title = g_strconcat (UGTK_APP_NAME " - ", _("Open Metalink file"), NULL);
	dialog = gtk_file_dialog_new ();
	gtk_file_dialog_set_title (dialog, title);
	g_free (title);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, "Metalink file (*.metalink, *.meta4)");
	gtk_file_filter_add_pattern (filter, "*.metalink");
	gtk_file_filter_add_pattern (filter, "*.meta4");
	filters = g_list_store_new (GTK_TYPE_FILE_FILTER);
	g_list_store_append (filters, filter);
	g_object_unref (filter);
	gtk_file_dialog_set_filters (dialog, G_LIST_MODEL (filters));
	g_object_unref (filters);

	gtk_file_dialog_open (dialog, app->window.self, NULL,
			on_create_metalink_done, app);
	g_object_unref (dialog);
}

// ------------------------------------
// import/export

static void  on_save_category_done (GObject* source, GAsyncResult* result, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	UgetNode* cnode;
	GFile*  gfile;
	gchar*  file;

	gtk_widget_set_sensitive ((GtkWidget*) app->window.self, TRUE);
	gfile = gtk_file_dialog_save_finish (GTK_FILE_DIALOG (source), result, NULL);
	if (gfile == NULL)
		return;
	if (app->traveler.category.cursor.pos == 0) {
		g_object_unref (gfile);
		return;
	}

	file = g_file_get_path (gfile);
	g_object_unref (gfile);
	cnode = app->traveler.category.cursor.node;
	if (uget_app_save_category ((UgetApp*) app, cnode->base, file, NULL) == FALSE)
		ugtk_app_show_message (app, TRUE, _("Failed to save category file."));
	g_free (file);
}

static void  on_load_category_done (GObject* source, GAsyncResult* result, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	GFile*  gfile;
	gchar*  file;

	gtk_widget_set_sensitive ((GtkWidget*) app->window.self, TRUE);
	gfile = gtk_file_dialog_open_finish (GTK_FILE_DIALOG (source), result, NULL);
	if (gfile == NULL)
		return;

	file = g_file_get_path (gfile);
	g_object_unref (gfile);
	if (uget_app_load_category ((UgetApp*) app, file, NULL))
		ugtk_menubar_sync_category (&app->menubar, app, TRUE);
	else
		ugtk_app_show_message (app, TRUE, _("Failed to load category file."));
	g_free (file);
}

void  ugtk_app_save_category (UgtkApp* app)
{
	GtkFileDialog*  dialog;
	gchar*          title;

	gtk_widget_set_sensitive ((GtkWidget*) app->window.self, FALSE);

	title = g_strconcat (UGTK_APP_NAME " - ", _("Save Category file"), NULL);
	dialog = gtk_file_dialog_new ();
	gtk_file_dialog_set_title (dialog, title);
	g_free (title);

	gtk_file_dialog_save (dialog, app->window.self, NULL,
			on_save_category_done, app);
	g_object_unref (dialog);
}

void  ugtk_app_load_category (UgtkApp* app)
{
	GtkFileDialog*  dialog;
	GtkFileFilter*  filter;
	GListStore*     filters;
	gchar*          title;

	gtk_widget_set_sensitive ((GtkWidget*) app->window.self, FALSE);

	title = g_strconcat (UGTK_APP_NAME " - ", _("Open Category file"), NULL);
	dialog = gtk_file_dialog_new ();
	gtk_file_dialog_set_title (dialog, title);
	g_free (title);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("JSON file (*.json)"));
	gtk_file_filter_add_mime_type (filter, "application/json");
	filters = g_list_store_new (GTK_TYPE_FILE_FILTER);
	g_list_store_append (filters, filter);
	g_object_unref (filter);
	gtk_file_dialog_set_filters (dialog, G_LIST_MODEL (filters));
	g_object_unref (filters);

	gtk_file_dialog_open (dialog, app->window.self, NULL,
			on_load_category_done, app);
	g_object_unref (dialog);
}

static void  on_import_html_file_done (GObject* source, GAsyncResult* result, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	UgHtmlFilter*     html_filter;
	UgHtmlFilterTag*  tag_a;
	UgHtmlFilterTag*  tag_img;
	UgtkBatchDialog*  bdialog;
	UgtkSelectorPage* page;
	UgetNode*  cnode;
	gchar*  string = NULL;
	gchar*  file;
	GFile*  gfile;

	gfile = gtk_file_dialog_open_finish (GTK_FILE_DIALOG (source), result, NULL);
	if (gfile == NULL)
		return;

	// read URLs from html file
	string = g_file_get_path (gfile);
	g_object_unref (gfile);
	file = g_filename_to_utf8 (string, -1, NULL, NULL, NULL);
	g_free (string);
	string = NULL;
	// parse html
	html_filter = ug_html_filter_new ();
	tag_a = ug_html_filter_tag_new ("A", "HREF");		// <A HREF="Link">
	ug_html_filter_add_tag (html_filter, tag_a);
	tag_img = ug_html_filter_tag_new ("IMG", "SRC");		// <IMG SRC="Link">
	ug_html_filter_add_tag (html_filter, tag_img);
	ug_html_filter_parse_file (html_filter, file);
	g_free (file);
	if (html_filter->base_href)
		string = g_strdup (html_filter->base_href);
	ug_html_filter_free (html_filter);
	// UgtkBatchDialog
	bdialog = ugtk_batch_dialog_new (
			UGTK_APP_NAME " - " "Import URLs from HTML file", app);
	ugtk_download_form_set_folders (&bdialog->download, &app->setting);
	ugtk_batch_dialog_use_selector (bdialog);
	// category
	cnode = app->traveler.category.cursor.node->base;
	if (cnode->parent != &app->real)
		cnode = app->real.children;
	ugtk_batch_dialog_set_category (bdialog, cnode);
	// set <base href>
	if (string) {
		gtk_editable_set_text (GTK_EDITABLE (bdialog->selector.href_entry), string);
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

static void  on_import_text_file_done (GObject* source, GAsyncResult* result, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	UgtkBatchDialog*   bdialog;
	UgtkSelectorPage*  page;
	UgetNode* cnode;
	gchar*    string;
	gchar*    file;
	GList*    list;
	GError*   error = NULL;
	GFile*    gfile;

	gfile = gtk_file_dialog_open_finish (GTK_FILE_DIALOG (source), result, NULL);
	if (gfile == NULL)
		return;

	// read URLs from text file
	string = g_file_get_path (gfile);
	g_object_unref (gfile);
	file = g_filename_to_utf8 (string, -1, NULL, NULL, NULL);
	g_free (string);
	list = ugtk_text_file_get_uris (file, &error);
	g_free (file);
	if (error) {
		ugtk_app_show_message (app, TRUE, error->message);
		g_error_free (error);
		return;
	}
	// UgtkBatchDialog
	bdialog = ugtk_batch_dialog_new (
			UGTK_APP_NAME " - " "Import URLs from text file", app);
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

static void  on_export_text_file_done (GObject* source, GAsyncResult* result, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	GIOChannel*  channel;
	UgetCommon*  common;
	UgetNode*    node;
	GFile*       gfile;
	gchar*       fname;

	gfile = gtk_file_dialog_save_finish (GTK_FILE_DIALOG (source), result, NULL);
	if (gfile == NULL)
		return;

	fname = g_file_get_path (gfile);
	g_object_unref (gfile);
	channel = g_io_channel_new_file (fname, "w", NULL);
	g_free (fname);

	if (channel == NULL)
		return;

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
	GtkFileDialog*  dialog;
	GtkFileFilter*  filter;
	GListStore*     filters;
	gchar*          title;

	title = g_strconcat (UGTK_APP_NAME " - ", _("Import URLs from HTML file"), NULL);
	dialog = gtk_file_dialog_new ();
	gtk_file_dialog_set_title (dialog, title);
	g_free (title);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("HTML file (*.htm, *.html)"));
	gtk_file_filter_add_mime_type (filter, "text/html");
	filters = g_list_store_new (GTK_TYPE_FILE_FILTER);
	g_list_store_append (filters, filter);
	g_object_unref (filter);
	gtk_file_dialog_set_filters (dialog, G_LIST_MODEL (filters));
	g_object_unref (filters);

	gtk_file_dialog_open (dialog, app->window.self, NULL,
			on_import_html_file_done, app);
	g_object_unref (dialog);
}

void  ugtk_app_import_text_file (UgtkApp* app)
{
	GtkFileDialog*  dialog;
	GtkFileFilter*  filter;
	GListStore*     filters;
	gchar*          title;

	title = g_strconcat (UGTK_APP_NAME " - ", _("Import URLs from text file"), NULL);
	dialog = gtk_file_dialog_new ();
	gtk_file_dialog_set_title (dialog, title);
	g_free (title);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Plain text file"));
	gtk_file_filter_add_mime_type (filter, "text/plain");
	filters = g_list_store_new (GTK_TYPE_FILE_FILTER);
	g_list_store_append (filters, filter);
	g_object_unref (filter);
	gtk_file_dialog_set_filters (dialog, G_LIST_MODEL (filters));
	g_object_unref (filters);

	gtk_file_dialog_open (dialog, app->window.self, NULL,
			on_import_text_file_done, app);
	g_object_unref (dialog);
}

void  ugtk_app_export_text_file (UgtkApp* app)
{
	GtkFileDialog*  dialog;
	gchar*          title;

	title = g_strconcat (UGTK_APP_NAME " - ", _("Export URLs to text file"), NULL);
	dialog = gtk_file_dialog_new ();
	gtk_file_dialog_set_title (dialog, title);
	g_free (title);

	gtk_file_dialog_save (dialog, app->window.self, NULL,
			on_export_text_file_done, app);
	g_object_unref (dialog);
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
		ugtk_app_show_message (app, TRUE,
				_("No URLs found in clipboard."));
		return;
	}
	// filter existing
	if (app->setting.ui.skip_existing) {
		if (ugtk_app_filter_existing (app, list) == 0) {
//			g_list_foreach (list, (GFunc) g_free, NULL);
			g_list_free (list);
			ugtk_app_show_message (app, FALSE,
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
// Refresh GListModel for UgtkNodeTree and UgtkNodeList

void  ugtk_app_download_changed (UgtkApp* app, UgetNode* dnode)
{
	ugtk_node_tree_refresh (app->traveler.download.model);
}

void  ugtk_app_category_changed (UgtkApp* app, UgetNode* cnode)
{
	ugtk_node_tree_refresh (app->traveler.category.model);
	// refresh status list
	ugtk_node_list_refresh (app->traveler.state.model);
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

void  ugtk_app_show_message (UgtkApp* app, gboolean is_error,
                             const gchar* message)
{
	GtkAlertDialog* alert;
	gchar*          title;

	if (is_error)
		title = g_strconcat (UGTK_APP_NAME " - ", _("Error"), NULL);
	else
		title = g_strconcat (UGTK_APP_NAME " - ", _("Message"), NULL);

	alert = gtk_alert_dialog_new ("%s", title);
	gtk_alert_dialog_set_detail (alert, message);
	gtk_alert_dialog_set_buttons (alert, (const char*[]){ _("_OK"), NULL });
	gtk_alert_dialog_show (alert, app->window.self);
	g_object_unref (alert);
	g_free (title);
}

void ugtk_app_open_settings(UgtkApp *app) {
    if (app->dialogs.setting == NULL) {
        app->dialogs.setting = (GtkWidget*) ugtk_setting_dialog_new(_("Settings"), (GtkWindow*)app->window.self);
    }
    ugtk_setting_dialog_set((UgtkSettingDialog*)app->dialogs.setting, &app->setting);
    ugtk_setting_dialog_run((UgtkSettingDialog*)app->dialogs.setting, app);
}

// -------------------------------------------------------
// UgtkClipboard

void  ugtk_clipboard_init (struct UgtkClipboard* clipboard, const gchar* pattern)
{
	clipboard->self  = gdk_display_get_clipboard (gdk_display_get_default ());
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
	if (clipboard->self)
		gdk_clipboard_set_text (clipboard->self, text);
}

GList* ugtk_clipboard_get_uris (struct UgtkClipboard* clipboard)
{
#if 0
	// GTK4: gtk_clipboard_wait_is_text_available and gtk_clipboard_wait_for_text removed
	// Needs rewrite using GdkClipboard async API: gdk_clipboard_read_text_async
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
#else
	// Temporary stub: clipboard monitoring disabled
	(void)clipboard;
	return NULL;
#endif
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
	if (n_selected > 0) {
		gchar* string = g_strdup_printf (_("Selected %d items"), n_selected);
		gtk_label_set_text (statusbar->info, string);
		g_free (string);
	}
	else {
		gtk_label_set_text (statusbar->info, "");
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

