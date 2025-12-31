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

#include <UgString.h>
#include <UgtkTrayIcon.h>
#include <UgtkAboutDialog.h>
#include <UgtkApp.h>

#include <glib/gi18n.h>

#define UGTK_TRAY_ICON_NAME         "uget-tray-default"
#define UGTK_TRAY_ICON_ERROR_NAME   "uget-tray-error"
#define UGTK_TRAY_ICON_ACTIVE_NAME  "uget-tray-downloading"

void ugtk_tray_icon_init (UgtkTrayIcon* trayicon)
{
	// Stubbed for GTK 4 port (GtkStatusIcon removed)
}

void ugtk_tray_icon_set_info (UgtkTrayIcon* trayicon, guint n_active, gint64 down_speed, gint64 up_speed)
{
	// Stubbed for GTK 4 port
}

void  ugtk_tray_icon_set_visible (UgtkTrayIcon* trayicon, gboolean visible)
{
	// Stubbed for GTK 4 port
}

#ifdef HAVE_APP_INDICATOR

void  ugtk_tray_icon_use_indicator (UgtkTrayIcon* trayicon, gboolean enable)
{
	ugtk_tray_icon_set_visible (trayicon, FALSE);
	if (enable)
		trayicon->indicator = trayicon->indicator_temp;
	else
		trayicon->indicator = NULL;
	ugtk_tray_icon_set_visible (trayicon, trayicon->visible);
}

#endif // HAVE_APP_INDICATOR

// ----------------------------------------------------------------------------
// Callback

#if 0
static void	on_trayicon_activate (GtkStatusIcon* status_icon, UgtkApp* app)
{
	gint  x, y;

	if (gtk_widget_get_visible ((GtkWidget*) app->window.self) == TRUE) {
		// get position and size
		gtk_window_get_position (app->window.self, &x, &y);
		gtk_window_get_size (app->window.self,
				&app->setting.window.width, &app->setting.window.height);
		// gtk_window_get_position() may return: x == -32000, y == -32000
		if (x + app->setting.window.width > 0)
			app->setting.window.x = x;
		if (y + app->setting.window.height > 0)
			app->setting.window.y = y;
		// hide window
#if defined _WIN32 || defined _WIN64
//		gtk_window_iconify (app->window.self);
#endif
		gtk_widget_hide ((GtkWidget*) app->window.self);
	}
	else {
#if defined _WIN32 || defined _WIN64
//		gtk_window_deiconify (app->window.self);
#endif
		gtk_widget_show ((GtkWidget*) app->window.self);
		gtk_window_present (app->window.self);
		ugtk_app_decide_trayicon_visible (app);
		// set position and size
		gtk_window_move (app->window.self,
				app->setting.window.x, app->setting.window.y);
		gtk_window_resize (app->window.self,
				app->setting.window.width, app->setting.window.height);
	}
	// clear error status
	if (app->trayicon.error_occurred) {
		app->trayicon.error_occurred = FALSE;
		app->trayicon.state = UGTK_TRAY_ICON_STATE_NORMAL;
		gtk_status_icon_set_from_icon_name (status_icon, UGTK_TRAY_ICON_NAME);
	}
}

static void	on_trayicon_popup_menu (GtkStatusIcon* status_icon, guint button,
                                    guint activate_time, UgtkTrayIcon* trayicon)
{
	gtk_menu_set_screen ((GtkMenu*) trayicon->menu.self,
			gtk_status_icon_get_screen (status_icon));
#if defined _WIN32 || defined _WIN64
	gtk_menu_popup ((GtkMenu*) trayicon->menu.self,
			NULL, NULL,
			NULL, NULL,
			button, activate_time);
#else
	gtk_menu_popup ((GtkMenu*) trayicon->menu.self,
			NULL, NULL,
			gtk_status_icon_position_menu, status_icon,
			button, activate_time);
#endif
}

#if defined _WIN32 || defined _WIN64
static gboolean	tray_menu_timeout (GtkMenu* menu)
{
	gtk_menu_popdown (menu);
	// return FALSE if the source should be removed.
	return FALSE;
}

static gboolean	tray_menu_leave_enter (GtkWidget* menu, GdkEventCrossing* event, gpointer data)
{
	static guint	tray_menu_timer = 0;

	if (event->type == GDK_LEAVE_NOTIFY &&
		(event->detail == GDK_NOTIFY_ANCESTOR || event->detail == GDK_NOTIFY_UNKNOWN))
	{
		if (tray_menu_timer == 0) {
			tray_menu_timer = g_timeout_add (500,
					(GSourceFunc) tray_menu_timeout, menu);
		}
	}
	else if (event->type == GDK_ENTER_NOTIFY && event->detail == GDK_NOTIFY_ANCESTOR)
	{
		if (tray_menu_timer != 0) {
			g_source_remove (tray_menu_timer);
			tray_menu_timer = 0;
		}
	}
	return FALSE;
}
#endif // _WIN32 || _WIN64

static void	on_create_download (GtkWidget* widget, UgtkApp* app)
{
	ugtk_app_create_download (app, NULL, NULL);
}

static void on_clipboard_monitor (GtkWidget* widget, UgtkApp* app)
{
	if (app->trayicon.menu.emission)
		g_signal_emit_by_name (app->menubar.edit.clipboard_monitor, "activate");
}

static void on_clipboard_quiet (GtkWidget* widget, UgtkApp* app)
{
	if (app->trayicon.menu.emission)
		g_signal_emit_by_name (app->menubar.edit.clipboard_quiet, "activate");
}

static void on_commandline_quiet (GtkWidget* widget, UgtkApp* app)
{
	if (app->trayicon.menu.emission)
		g_signal_emit_by_name (app->menubar.edit.commandline_quiet, "activate");
}

static void on_skip_existing (GtkWidget* widget, UgtkApp* app)
{
	if (app->trayicon.menu.emission)
		g_signal_emit_by_name (app->menubar.edit.skip_existing, "activate");
}

static void on_apply_recent (GtkWidget* widget, UgtkApp* app)
{
	if (app->trayicon.menu.emission)
		g_signal_emit_by_name (app->menubar.edit.apply_recent, "activate");
}

static void  on_config_settings (GtkWidget* widget, UgtkApp* app)
{
	g_signal_emit_by_name (app->menubar.edit.settings, "activate");
}

static void  on_offline_mode (GtkWidget* widget, UgtkApp* app)
{
	gboolean  offline;

	offline = gtk_check_menu_item_get_active ((GtkCheckMenuItem*) widget);
	app->setting.offline_mode = offline;
	// file menu
	offline = gtk_check_menu_item_get_active (
			(GtkCheckMenuItem*) app->menubar.file.offline_mode);
	if (offline != app->setting.offline_mode) {
		gtk_check_menu_item_set_active (
				(GtkCheckMenuItem*) app->menubar.file.offline_mode,
				app->setting.offline_mode);
	}
}

static void  on_about (GtkWidget* widget, UgtkApp* app)
{
	g_signal_emit_by_name (app->menubar.help.about_uget, "activate");
}

#ifdef HAVE_APP_INDICATOR
static void	on_trayicon_show_window (GtkWidget* widget, UgtkApp* app)
{
	if (gtk_widget_get_visible ((GtkWidget*) app->window.self))
		gtk_window_present (app->window.self);
	else {
		gtk_widget_show ((GtkWidget*) app->window.self);
//		gtk_window_deiconify (app->window.self);
		gtk_window_present (app->window.self);
		ugtk_app_decide_trayicon_visible (app);
	}
}
#endif // HAVE_APP_INDICATOR
#endif // 0 - Stubbed callbacks


void  ugtk_trayicon_init_callback (struct UgtkTrayIcon* trayicon, UgtkApp* app)
{
	// Stubbed for GTK 4 port
}

