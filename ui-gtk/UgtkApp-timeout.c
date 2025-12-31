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

#ifdef	HAVE_CONFIG_H
#include <config.h>
#  if HAVE_LIBNOTIFY
#    include <libnotify/notify.h>
#  endif
#  if HAVE_GSTREAMER
#    include <gst/gst.h>
#  endif
#endif

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif // _MSC_VER

#if defined _WIN32 || defined _WIN64
#ifndef	_WIN32_IE
#define	_WIN32_IE	0x0600
#endif // _WIN32_IE
#include <windows.h>
#include <mmsystem.h>
#endif // _WIN32 || _WIN64

#include <UgUtil.h>
#include <UgtkApp.h>
#include <UgtkUtil.h>
#include <UgtkBatchDialog.h>

#include <glib/gi18n.h>

static void ugtk_app_notify_error (UgtkApp* app);
static void ugtk_app_notify_starting (UgtkApp* app);
static void ugtk_app_notify_completed (UgtkApp* app);
// GSourceFunc
#ifdef HAVE_RSS_NOTIFY
static gboolean  ugtk_app_timeout_rss (UgtkApp* app);
#endif
static gboolean  ugtk_app_timeout_rpc (UgtkApp* app);
static gboolean  ugtk_app_timeout_queuing (UgtkApp* app);
static gboolean  ugtk_app_timeout_clipboard (UgtkApp* app);
static gboolean  ugtk_app_timeout_autosave (UgtkApp* app);

void  ugtk_app_init_timeout (UgtkApp* app)
{
	// 0.5 seconds
	g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, 500,
			(GSourceFunc) ugtk_app_timeout_rpc, app, NULL);
	// 0.5 seconds
	g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, 500,
			(GSourceFunc) ugtk_app_timeout_queuing, app, NULL);
	// 2 seconds
	g_timeout_add_seconds_full (G_PRIORITY_DEFAULT_IDLE, 2,
			(GSourceFunc) ugtk_app_timeout_clipboard, app, NULL);
#ifdef HAVE_RSS_NOTIFY
	// 3 seconds
	g_timeout_add_seconds_full (G_PRIORITY_DEFAULT_IDLE, 3,
			(GSourceFunc) ugtk_app_timeout_rss, app, NULL);
#endif  // HAVE_RSS_NOTIFY
	// 1 minutes
	g_timeout_add_seconds_full (G_PRIORITY_DEFAULT_IDLE, 60,
			(GSourceFunc) ugtk_app_timeout_autosave, app, NULL);
}

static gboolean  ugtk_app_timeout_autosave (UgtkApp* app)
{
	static int  counts = 0;

	counts++;
	// "app->setting.auto_save.interval" may changed by user
	if (counts >= app->setting.auto_save.interval) {
		counts = 0;
		if (app->setting.auto_save.enable)
			ugtk_app_save (app);
	}
	// return FALSE if the source should be removed.
	return TRUE;
}

// ----------------------------------------------------------------------------
// Queuing

// scheduler
static gboolean  ugtk_app_decide_schedule_state (UgtkApp* app)
{
	struct tm*	timem;
	time_t		timet;
	guint		weekdays, dayhours;
	gboolean	changed;
	UgetNode*   cnode;
	UgtkScheduleState	 state;

	if (app->setting.scheduler.enable == FALSE) {
		app->schedule_state = UGTK_SCHEDULE_NORMAL;
		return FALSE;
	}

	// get current time
	timet = time (NULL);
	timem = localtime (&timet);
	dayhours = timem->tm_hour;
	if (timem->tm_wday == 0)
		weekdays = 6;
	else
		weekdays = timem->tm_wday - 1;
	// get current schedule state
	state = app->setting.scheduler.state.at[weekdays*24 + dayhours];
	if (app->schedule_state == state)
		changed = FALSE;
	else {
		app->schedule_state  = state;
		changed = TRUE;
		// switch mode
		switch (state) {
		case UGTK_SCHEDULE_TURN_OFF:
			for (cnode = app->real.children;  cnode;  cnode = cnode->next)
				uget_app_stop_category ((UgetApp*)app, cnode);
			// Don't notify anything
			app->user_action = TRUE;
			break;

		case UGTK_SCHEDULE_LIMITED_SPEED:
//			ug_running_set_speed (&app->running,
//					app->setting.speed_limit.scheduler.download, 0);
			break;

		default:
			// no speed limit
			uget_task_set_speed (&app->task, 0, 0);
			break;
		}
	}

	if (changed) {
		// refresh other data & status
		gtk_widget_queue_draw ((GtkWidget*) app->traveler.download.view);
		gtk_widget_queue_draw ((GtkWidget*) app->traveler.category.view);
		gtk_widget_queue_draw ((GtkWidget*) app->traveler.state.view);
	}
	return changed;
}

static gboolean  ugtk_app_timeout_queuing (UgtkApp* app)
{
	static int  n_counts = 0;
	static int  n_active_last = 0;
	int         n_active;
	int         no_queuing = FALSE;
	gchar*      string;

	ugtk_app_decide_schedule_state (app);
	if (app->setting.offline_mode ||
	    app->schedule_state == UGTK_SCHEDULE_TURN_OFF)
	{
		no_queuing = TRUE;
	}

	// if current status is "All" or "Active"
	if (app->traveler.state.cursor.pos == 0 ||
	    app->traveler.state.cursor.pos == 1)
	{
		ugtk_traveler_reserve_selection (&app->traveler);
	}

	n_active = uget_app_grow ((UgetApp*) app, no_queuing);
	if (n_active != n_active_last) {
		// start downloading
		if (n_active > 0 && n_active_last == 0) {
			// starting notification
			if (app->setting.ui.start_notification)
				ugtk_app_notify_starting (app);
		}
		// stop downloading
		else if (n_active == 0 && n_active_last > 0) {
			if (app->n_error > 0) {
				// error notification
				ugtk_app_notify_error (app);
				// This will show error icon at tray.
				app->trayicon.error_occurred = TRUE;
			}
			else if (app->user_action == FALSE) {
				// completed notification
				ugtk_app_notify_completed (app);
				// This will show normal icon at tray.
				app->trayicon.error_occurred = FALSE;
			}
			else {
				// This will show normal icon at tray.
				app->trayicon.error_occurred = FALSE;
			}
			// Completion Auto-Actions
			if (app->setting.completion.action > 0 && app->user_action == FALSE) {
				ugtk_app_save (app);
				switch (app->setting.completion.action) {
				case 1:    // hibernate
					ug_hibernate ();
					break;
				case 2:    // suspend
					ug_suspend ();
					break;
				case 3:    // shutdown
					ug_shutdown ();
					break;
				case 4:    // reboot
					ug_reboot ();
					break;
				case 5:    // custom
					if (app->n_error > 0) {
						if (app->setting.completion.on_error)
							if (system (app->setting.completion.on_error)) {}
					}
					else {
						if (app->setting.completion.command)
							if (system (app->setting.completion.command)) {}
					}
					break;
				}
			}
			// reset counter
			app->n_error = 0;
			app->n_completed = 0;
		}
		// change window title
		if (n_active == 0)
			gtk_window_set_title (app->window.self, UGTK_APP_NAME);
		else {
			string = g_strdup_printf (UGTK_APP_NAME " - %u %s",
					n_active, _("tasks"));
			gtk_window_set_title (app->window.self, string);
			g_free (string);
		}
	}

	if (n_active > 0 || n_active != n_active_last || app->n_moved > 0) {
		// adjust speed limit every 1 seconds
		if (n_counts & 1)
			uget_task_adjust_speed (&app->task);
		// summary
		ugtk_summary_show (&app->summary,
				ugtk_traveler_get_cursor (&app->traveler));
		// status bar
		ugtk_statusbar_set_speed (&app->statusbar,
				app->task.speed.download, app->task.speed.upload);
		// tray icon
		ugtk_tray_icon_set_info (&app->trayicon, n_active,
				app->task.speed.download, app->task.speed.upload);
	}

	// if current status is "All" or "Active"
	if (app->traveler.state.cursor.pos == 0 ||
	    app->traveler.state.cursor.pos == 1)
	{
		if (app->n_moved > 0)
			ugtk_traveler_restore_selection (&app->traveler);
	}

	uget_app_trim((UgetApp*) app, NULL);
	if (app->n_moved > 0) {
		// refresh other data & status
		gtk_widget_queue_draw ((GtkWidget*) app->traveler.download.view);
		gtk_widget_queue_draw ((GtkWidget*) app->traveler.category.view);
		gtk_widget_queue_draw ((GtkWidget*) app->traveler.state.view);
	}

	app->user_action = FALSE;
	app->n_moved = 0;   // reset counter
	n_active_last = n_active;
	n_counts++;
	return TRUE;
}

// ----------------------------------------------------------------------------
// Clipboard
//

static void  on_keep_above_window_show (GtkWindow *window, gpointer  user_data)
{
	gtk_window_present (window);
//	gtk_window_set_keep_above (window, FALSE);
}

static void  ugtk_app_add_uris_quietly (UgtkApp* app,       GList* list,
                                        UgInfo*  node_info, int  nth_category)
{
	GList*      link;
	UgUri       uuri;
	UgetNode*   cnode;
	UgetNode*   dnode;
	UgetCommon* common;

	// filter existing
	if (app->setting.ui.skip_existing) {
		if (ugtk_app_filter_existing (app, list) == 0)
			return;
	}

	for (link = list;  link;  link = link->next) {
		// check existing
		if (link->data == NULL)
			continue;
		// select category
		cnode = NULL;
		if (nth_category != -1)
			cnode = uget_node_nth_child (&app->real, nth_category);
		if (cnode == NULL) {
			// match category by URI
			ug_uri_init (&uuri, link->data);
			cnode = uget_app_match_category ((UgetApp*) app, &uuri, NULL);
		}
		if (cnode == NULL && node_info) {
			// match category by filename
			common = ug_info_realloc(node_info, UgetCommonInfo);
			if (common && common->file) {
				ug_uri_init (&uuri, common->file);
				cnode = uget_app_match_category ((UgetApp*) app, &uuri, NULL);
			}
		}
		if (cnode == NULL) {
			if (node_info == NULL) {
				cnode = uget_node_nth_child (&app->real,
						app->setting.clipboard.nth_category);
			}
			else {
				cnode = uget_node_nth_child (&app->real,
						app->setting.commandline.nth_category);
			}
		}
		if (cnode == NULL)
			cnode = uget_node_nth_child (&app->real, 0);
		// add download
		if (node_info == NULL) {
			// add and free URI
			uget_app_add_download_uri ((UgetApp*) app, link->data, cnode, TRUE);
			g_free (link->data);
		}
		else {
			dnode = uget_node_new (NULL);
			ug_info_assign(dnode->info, node_info, NULL);
			common = ug_info_realloc(dnode->info, UgetCommonInfo);
			common->uri = link->data;
			uget_app_add_download ((UgetApp*) app, dnode, cnode, TRUE);
		}
		link->data = NULL;
	}
}

static void  ugtk_app_add_uris_selected (UgtkApp* app,       GList* list,
                                         UgInfo*  node_info, int  nth_category)
{
	UgtkBatchDialog*    bdialog;
	UgtkSelectorPage*   page;
	UgetCommon*         common;
	UgetNode*           cnode;
	UgUri               uuri;
	gchar*              title;

	// filter existing
	if (app->setting.ui.skip_existing) {
		if (ugtk_app_filter_existing (app, list) == 0)
			return;
	}

	// choose title for clipboard or command-line
	if (node_info == NULL)
		title = g_strconcat (UGTK_APP_NAME " - ", _("New from Clipboard"), NULL);
	else
		title = g_strconcat (UGTK_APP_NAME " - ", _("New Download"), NULL);
	bdialog = ugtk_batch_dialog_new (title, app);
	g_free (title);

	// disable batch if only one uri in list.
	if (list->next == NULL)
		ugtk_batch_dialog_disable_batch (bdialog);
	// apply other setting
	if (node_info) {
		ugtk_download_form_set(&bdialog->download, node_info, FALSE);
		ugtk_proxy_form_set(&bdialog->proxy, node_info, FALSE);
	}
	// add URIs
	if (list->next == NULL) {
		gtk_editable_set_text (GTK_EDITABLE (bdialog->download.uri_entry),
		                    list->data);
		bdialog->download.changed.uri = TRUE;
	}
	else {
		// After calling ugtk_selector_page_add_uris(),
		// all uris in list will be freed.
		ugtk_batch_dialog_use_selector (bdialog);
		ugtk_selector_hide_href (&bdialog->selector);
		if (node_info == NULL)
			page = ugtk_selector_add_page (&bdialog->selector, _("Clipboard"));
		else
			page = ugtk_selector_add_page (&bdialog->selector, _("Command line"));
		ugtk_selector_page_add_uris (page, list);
	}
	// set folder history and info
	ugtk_download_form_set_folders (&bdialog->download, &app->setting);
	// select category
	cnode = NULL;
	if (nth_category != -1)
		cnode = uget_node_nth_child (&app->real, nth_category);
	if (cnode == NULL && list->data) {
		// match category by URI
		ug_uri_init (&uuri, list->data);
		if (node_info == NULL)
			cnode = uget_app_match_category ((UgetApp*) app, &uuri, NULL);
		else {
			common = ug_info_realloc(node_info, UgetCommonInfo);
			cnode = uget_app_match_category ((UgetApp*) app, &uuri, common->file);
		}
	}
	if (cnode == NULL) {
		if (node_info == NULL) {
			cnode = uget_node_nth_child (&app->real,
					app->setting.clipboard.nth_category);
		}
		else {
			cnode = uget_node_nth_child (&app->real,
					app->setting.commandline.nth_category);
		}
	}
	if (cnode == NULL)
		cnode = uget_node_nth_child (&app->real, 0);
	ugtk_batch_dialog_set_category (bdialog, cnode);

	// connect signal and set data in download dialog
	g_signal_connect_after (bdialog->self, "show",
			G_CALLBACK (on_keep_above_window_show), NULL);
	// Make sure dilaog will show on top first time.
	// uget_on_keep_above_window_show ()  will set keep_above = FALSE
	// gtk_window_set_keep_above ((GtkWindow*) bdialog->self, TRUE);
	ugtk_batch_dialog_run (bdialog);
}

static void on_clipboard_text_received (GdkClipboard*  clipboard,
                                        const gchar*   text,
                                        gpointer       user_data)
{
#if 0
	UgtkApp*  app;
	GList*    list;

	app = (UgtkApp*) user_data;
	list = ugtk_clipboard_get_matched (&app->clipboard, text);
	if (list) {
		if (app->setting.clipboard.quiet)
			ugtk_app_add_uris_quietly (app, list, NULL, -1);
		else
			ugtk_app_add_uris_selected (app, list, NULL, -1);
		g_list_free (list);
		// refresh
		gtk_widget_queue_draw ((GtkWidget*) app->traveler.category.view);
		gtk_widget_queue_draw ((GtkWidget*) app->traveler.state.view);
	}

	app->clipboard.processing = FALSE;
#endif
}

static gboolean  ugtk_app_timeout_clipboard (UgtkApp* app)
{
#if 0
	if (app->setting.clipboard.monitor && app->clipboard.processing == FALSE) {
		// set FALSE in on_clipboard_text_received()
		app->clipboard.processing = TRUE;
		gtk_clipboard_request_text (app->clipboard.self,
				on_clipboard_text_received, app);
	}
#endif
	// return FALSE if the source should be removed.
	return TRUE;
}

// ----------------------------------------------------------------------------
// RPC

static gboolean  ugtk_app_timeout_rpc (UgtkApp* app)
{
	UgetRpcReq*  req;
	UgetRpcCmd*  cmd;
	UgInfo*      node_info;

	for (;;) {
		if (uget_rpc_has_request(app->rpc) == FALSE)
			return TRUE;

		req = uget_rpc_get_request (app->rpc);
		switch (req->method_id) {
		case UGET_RPC_PRESENT:
//			if (gtk_widget_get_visible ((GtkWidget*) app->window.self) == FALSE)
//				gtk_window_deiconify (app->window.self);
			gtk_window_present (app->window.self);
			// set position and size
			/*
			gtk_window_move (app->window.self,
					app->setting.window.x, app->setting.window.y);
			gtk_window_resize (app->window.self,
					app->setting.window.width, app->setting.window.height);
			*/
			break;

		case UGET_RPC_SEND_COMMAND:
			cmd = (UgetRpcCmd*) req;
			// control online/offline
			switch (cmd->value.ctrl.offline) {
			case 0:
				app->setting.offline_mode = FALSE;
				/*
				gtk_check_menu_item_set_active (
						(GtkCheckMenuItem*) app->menubar.file.offline_mode,
						FALSE);
				*/
				break;

			case 1:
				app->setting.offline_mode = TRUE;
				/*
				gtk_check_menu_item_set_active (
						(GtkCheckMenuItem*) app->menubar.file.offline_mode,
						TRUE);
				*/
				break;

			default:
				break;
			}

			// if user want to add downloads quietly.
			if (app->setting.commandline.quiet)
				cmd->value.quiet = TRUE;

			// URIs
			if (cmd->uris.size == 0)
				break;

			// add downloads
			node_info = ug_info_new(8, 0);
			uget_option_value_to_info(&cmd->value, node_info);
			if (cmd->value.quiet) {
				ugtk_app_add_uris_quietly (app, (GList*) cmd->uris.head,
						node_info, cmd->value.category_index);
			}
			else {
				ugtk_app_add_uris_selected (app, (GList*) cmd->uris.head,
						node_info, cmd->value.category_index);
			}
			ug_info_unref(node_info);
			break;

		default:
			break;
		}
		req->free (req);
	}

	return TRUE;
}

// ----------------------------------------------------------------------------
// RSS

#ifdef HAVE_RSS_NOTIFY
static gboolean  ugtk_app_timeout_rss_update (UgtkApp* app)
{
	uget_rss_update (app->rss_builtin, FALSE);
	// check RSS ready every 3 seconds
	g_timeout_add_seconds_full (G_PRIORITY_DEFAULT_IDLE, 3,
			(GSourceFunc) ugtk_app_timeout_rss, app, NULL);
	// return FALSE if the source should be removed.
	return FALSE;
}

static gboolean  ugtk_app_timeout_rss (UgtkApp* app)
{
	if (app->rss_builtin->updating == FALSE) {
		if (app->rss_builtin->n_updated > 0) {
			ugtk_banner_show_rss (&app->banner, app->rss_builtin);
			app->rss_builtin->n_updated = 0;
		}
		// update RSS every 30 minutes
		g_timeout_add_seconds_full (G_PRIORITY_DEFAULT_IDLE, 60 * 30,
				(GSourceFunc) ugtk_app_timeout_rss_update, app, NULL);
		// return FALSE if the source should be removed.
		return FALSE;
	}
	return TRUE;
}
#endif  // HAVE_RSS_NOTIFY

// ----------------------------------------------------------------------------
// sound

// static void uget_play_sound (const gchar* sound_file);
// GStreamer
#ifdef HAVE_GSTREAMER
static gboolean ugst_bus_func (GstBus* bus, GstMessage* msg, gpointer data)
{
	GstElement*	playbin = data;
	GError*		error   = NULL;

	switch (GST_MESSAGE_TYPE (msg)) {
	case GST_MESSAGE_WARNING:
		gst_message_parse_warning (msg, &error, NULL);
//		g_print ("uget-gtk: gstreamer: %s\n", error->message);
		g_error_free (error);
		break;

	case GST_MESSAGE_ERROR:
		gst_message_parse_error (msg, &error, NULL);
//		g_print ("uget-gtk: gstreamer: %s\n", error->message);
		g_error_free (error);
		// clean up
	case GST_MESSAGE_EOS:
		gst_element_set_state (playbin, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (playbin));
		break;

	default:
		break;
	}
	return TRUE;
}

static void ugtk_play_sound (const gchar* sound_file)
{
	GstElement*	playbin = NULL;
	GstBus*		bus     = NULL;
	char*		uri;
	gchar*		file_os;
	extern gboolean	gst_inited;		// ui-gtk/UgtkApp-main.c

	if (gst_inited == FALSE)
		return;

	file_os = g_filename_from_utf8 (sound_file, -1, NULL, NULL, NULL);
	if (g_file_test (file_os, G_FILE_TEST_EXISTS) == FALSE) {
		g_free (file_os);
		return;
	}

	playbin = gst_element_factory_make ("playbin", "play");
	if (playbin == NULL) {
		g_free (file_os);
		return;
	}

	uri = g_filename_to_uri (file_os, NULL, NULL);
	g_free (file_os);

	g_object_set (G_OBJECT (playbin), "uri", uri, NULL);

	bus = gst_pipeline_get_bus (GST_PIPELINE (playbin));
	gst_bus_add_watch (bus, ugst_bus_func, playbin);

	gst_element_set_state (playbin, GST_STATE_PLAYING);

	gst_object_unref (bus);
	g_free (uri);
}

#elif defined (_WIN32)
static void ugtk_play_sound (const gchar* sound_file)
{
	gunichar2*	file_wcs;

	if (g_file_test (sound_file, G_FILE_TEST_EXISTS) == FALSE)
		return;

	file_wcs = g_utf8_to_utf16 (sound_file, -1, NULL, NULL, NULL);
	PlaySoundW (file_wcs, NULL, SND_ASYNC | SND_FILENAME);
	g_free (file_wcs);
}

#else
// --disable-gstreamer
static void ugtk_play_sound (const gchar* sound_file)
{
}
#endif	// HAVE_GSTREAMER

// ----------------------------------------------------------------------------
// notification
//
#if defined HAVE_LIBNOTIFY
static void ugtk_app_notify (UgtkApp* app, const gchar* title, const gchar* body)
{
	static	NotifyNotification*	notification = NULL;
	gchar*	string;

	if (notify_is_initted () == FALSE)
		return;
	// set title and body
	string = g_strconcat (UGTK_APP_NAME " - ", title, NULL);
	if (notification == NULL) {

#if defined (NOTIFY_VERSION_MINOR) && NOTIFY_VERSION_MAJOR >= 0 && NOTIFY_VERSION_MINOR >= 7
		notification = notify_notification_new (string,
				body, UGTK_APP_ICON_NAME);
#else
		notification = notify_notification_new (string,
				body, UGTK_APP_ICON_NAME, NULL);
#endif
		notify_notification_set_timeout (notification, 7000);	// milliseconds
	}
	else {
		notify_notification_update (notification, string,
				body, UGTK_APP_ICON_NAME);
	}
	g_free (string);

	notify_notification_show (notification, NULL);
}
#elif defined _WIN32 || defined _WIN64
static void ugtk_app_notify (UgtkApp* app, const gchar* title, const gchar* body)
{
	static	NOTIFYICONDATAW*	pNotifyData = NULL;
	gchar*		string;
	gunichar2*	string_wcs;

	if (pNotifyData == NULL) {
		pNotifyData = g_malloc0 (sizeof (NOTIFYICONDATAW));
		pNotifyData->cbSize = sizeof (NOTIFYICONDATAW);
		pNotifyData->uFlags = NIF_INFO;			// Use a balloon ToolTip instead of a standard ToolTip.
		pNotifyData->uTimeout = 7000;			// milliseconds, This member is deprecated as of Windows Vista.
		pNotifyData->dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;	// Add an information icon to balloon ToolTip.
		// gtkstatusicon.c
		// (create_tray_observer): WNDCLASS.lpszClassName = "gtkstatusicon-observer"
		pNotifyData->hWnd = FindWindowA ("gtkstatusicon-observer", NULL);
	}

    /*
	if (pNotifyData->hWnd == NULL)
		return;
	// gtkstatusicon.c
	// (gtk_status_icon_init): priv->nid.uID = GPOINTER_TO_UINT (status_icon);
	// pNotifyData->uID = GPOINTER_TO_UINT (app->trayicon.self);
	// title
	string = g_strconcat (UGTK_APP_NAME " - ", title, NULL);
	string_wcs = g_utf8_to_utf16 (string,  -1, NULL, NULL, NULL);
	wcsncpy (pNotifyData->szInfoTitle, string_wcs, 64 -1);	// null-terminated
	g_free (string);
	g_free (string_wcs);
	// body
	string_wcs = g_utf8_to_utf16 (body, -1, NULL, NULL, NULL);
	wcsncpy (pNotifyData->szInfo, string_wcs, 256 -1);	// null-terminated
	g_free (string_wcs);

	Shell_NotifyIconW (NIM_MODIFY, pNotifyData);
    */
}
#else
static void ugtk_app_notify (UgtkApp* app, const gchar* title, const gchar* info)
{
	// do nothing
}
#endif	// HAVE_LIBNOTIFY

#define	NOTIFICATION_ERROR_TITLE       _("Error Occurred")
#define	NOTIFICATION_ERROR_STRING      _("Error Occurred during downloading.")
#define	NOTIFICATION_STARTING_TITLE    _("Download Starting")
#define	NOTIFICATION_STARTING_STRING   _("Starting download queue.")
#define	NOTIFICATION_COMPLETED_TITLE   _("Download Completed")
#define	NOTIFICATION_COMPLETED_STRING  _("All queuing downloads have been completed.")

static void ugtk_app_notify_error (UgtkApp* app)
{
	gchar*	path;

	ugtk_app_notify (app,
			NOTIFICATION_ERROR_TITLE,
			NOTIFICATION_ERROR_STRING);

	if (app->setting.ui.sound_notification) {
		path = g_build_filename (ugtk_get_data_dir (),
				"sounds", "uget", "notification.wav",  NULL);
		ugtk_play_sound (path);
		g_free (path);
	}
}

static void ugtk_app_notify_completed (UgtkApp* app)
{
	gchar*	path;

	ugtk_app_notify (app,
			NOTIFICATION_COMPLETED_TITLE,
			NOTIFICATION_COMPLETED_STRING);

	if (app->setting.ui.sound_notification) {
		path = g_build_filename (ugtk_get_data_dir (),
				"sounds", "uget", "notification.wav",  NULL);
		ugtk_play_sound (path);
		g_free (path);
	}
}

static void ugtk_app_notify_starting (UgtkApp* app)
{
//	gchar*	path;

	ugtk_app_notify (app,
			NOTIFICATION_STARTING_TITLE,
			NOTIFICATION_STARTING_STRING);

//	if (app->setting.ui.sound_notification) {
//		path = g_build_filename (ugtk_get_data_dir (),
//				"sounds", "uget", "notification.wav",  NULL);
//		ugtk_play_sound (path);
//		g_free (path);
//	}
}

