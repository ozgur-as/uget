/*
 *
 *   Copyright (C) 2026 by Ozgur As
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
 */

// Windows tray icon, the Shell_NotifyIcon counterpart of the
// StatusNotifierItem implementation in UgtkTrayIcon.c. Exactly one of the two
// is compiled (see ui-gtk/meson.build); both provide the same public API.
//
// Balloon notifications live here too rather than in UgtkApp-timeout.c,
// because on Windows a notification is a property of the tray icon: it is a
// NIM_MODIFY carrying NIF_INFO on this same NOTIFYICONDATAW.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// szInfo/szInfoTitle (the balloon fields of NOTIFYICONDATAW) are compiled out
// unless _WIN32_IE is at least 0x0500.
#ifndef _WIN32_IE
#define _WIN32_IE   0x0600
#endif

// winsock2.h must precede windows.h: UgtkApp.h reaches UgSocket.h (and so
// ws2tcpip.h) further down, and windows.h would otherwise pull in winsock v1.
#include <winsock2.h>
#include <windows.h>
#include <shellapi.h>

#include <string.h>     // memset()

#include <glib/gi18n.h>

#include <UgString.h>
#include <UgtkTrayIcon.h>
#include <UgtkApp.h>

// Private message posted by the shell when the icon is clicked.
#define UGTK_TRAY_MESSAGE   (WM_APP + 1)

#define UGTK_TRAY_CLASS     L"uGetTrayWindow"

// Menu command identifiers. Order matches the menu built in build_menu().
enum
{
	CMD_NEW_DOWNLOAD = 1,
	CMD_NEW_CLIPBOARD,
	CMD_NEW_TORRENT,
	CMD_NEW_METALINK,
	CMD_CLIPBOARD_MONITOR,
	CMD_CLIPBOARD_QUIET,
	CMD_COMMANDLINE_QUIET,
	CMD_SKIP_EXISTING,
	CMD_APPLY_RECENT,
	CMD_SETTINGS,
	CMD_ABOUT,
	CMD_SHOW_WINDOW,
	CMD_OFFLINE_MODE,
	CMD_QUIT,
};

// There is only ever one tray icon, so the Win32 handles live here rather
// than in UgtkTrayIcon -- that keeps windows.h out of UgtkTrayIcon.h.
static UgtkApp*         tray_app       = NULL;
static HWND             tray_hwnd      = NULL;
static HMENU            tray_menu      = NULL;
static NOTIFYICONDATAW  tray_nid;
static gboolean         tray_added     = FALSE;
static UINT             msg_taskbar_created = 0;

// ----------------------------------------------------------------------------
// Helpers

// Caller frees with g_free().
static wchar_t* utf8_to_wcs (const gchar* text)
{
	return (wchar_t*) g_utf8_to_utf16 (text ? text : "", -1, NULL, NULL, NULL);
}

static void wcs_copy (wchar_t* dest, size_t dest_len, const gchar* text)
{
	wchar_t* wcs = utf8_to_wcs (text);

	if (wcs) {
		wcsncpy (dest, wcs, dest_len - 1);
		dest[dest_len - 1] = L'\0';
		g_free (wcs);
	}
	else
		dest[0] = L'\0';
}

static void menu_append (HMENU menu, UINT id, const gchar* label)
{
	wchar_t* wcs = utf8_to_wcs (label);

	AppendMenuW (menu, MF_STRING, id, wcs);
	g_free (wcs);
}

// Icons are embedded as resources (see packaging/windows/uget.rc.in), so the
// tray works regardless of how the application is installed.
static HICON load_state_icon (guint state)
{
	const wchar_t* name;
	int            cx = GetSystemMetrics (SM_CXSMICON);
	int            cy = GetSystemMetrics (SM_CYSMICON);

	switch (state) {
	case UGTK_TRAY_ICON_STATE_ERROR:    name = L"TRAYERROR";       break;
	case UGTK_TRAY_ICON_STATE_RUNNING:  name = L"TRAYDOWNLOADING"; break;
	default:                            name = L"TRAYDEFAULT";     break;
	}

	return (HICON) LoadImageW (GetModuleHandleW (NULL), name,
	                           IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR);
}

// Mirrors on_show_window() in the StatusNotifierItem implementation.
static void show_main_window (UgtkApp* app)
{
	if (gtk_widget_get_visible ((GtkWidget*) app->window.self))
		gtk_window_present (app->window.self);
	else {
		gtk_widget_set_visible ((GtkWidget*) app->window.self, TRUE);
		gtk_window_present (app->window.self);
		ugtk_app_decide_trayicon_visible (app);
	}
}

// Flip a checkable item and report its new state.
static gboolean toggle_menu_item (UINT id)
{
	gboolean checked = (GetMenuState (tray_menu, id, MF_BYCOMMAND) & MF_CHECKED) != 0;

	CheckMenuItem (tray_menu, id, MF_BYCOMMAND | (checked ? MF_UNCHECKED : MF_CHECKED));
	return !checked;
}

// Keep the menubar's GAction in step, as the dbusmenu callbacks do.
static void sync_action (UgtkApp* app, const gchar* action_name, gboolean active)
{
	GAction* action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group),
	                                              action_name);
	if (action)
		g_action_change_state (action, g_variant_new_boolean (active));
}

static void activate_action (UgtkApp* app, const gchar* action_name)
{
	GAction* action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group),
	                                              action_name);
	if (action)
		g_action_activate (action, NULL);
}

// ----------------------------------------------------------------------------
// Menu

static void build_menu (void)
{
	tray_menu = CreatePopupMenu ();

	// Section 1: Create downloads
	menu_append (tray_menu, CMD_NEW_DOWNLOAD,  _("New Download..."));
	menu_append (tray_menu, CMD_NEW_CLIPBOARD, _("New Clipboard batch..."));
	AppendMenuW (tray_menu, MF_SEPARATOR, 0, NULL);

	// Section 2: Torrent/Metalink
	menu_append (tray_menu, CMD_NEW_TORRENT,  _("New Torrent..."));
	menu_append (tray_menu, CMD_NEW_METALINK, _("New Metalink..."));
	AppendMenuW (tray_menu, MF_SEPARATOR, 0, NULL);

	// Section 3: Setting toggles
	menu_append (tray_menu, CMD_CLIPBOARD_MONITOR,  _("Clipboard Monitor"));
	menu_append (tray_menu, CMD_CLIPBOARD_QUIET,    _("Clipboard works quietly"));
	menu_append (tray_menu, CMD_COMMANDLINE_QUIET,  _("Command-line works quietly"));
	menu_append (tray_menu, CMD_SKIP_EXISTING,      _("Skip existing URI"));
	menu_append (tray_menu, CMD_APPLY_RECENT,       _("Apply recent download settings"));
	AppendMenuW (tray_menu, MF_SEPARATOR, 0, NULL);

	// Section 4: Settings and About
	menu_append (tray_menu, CMD_SETTINGS, _("Settings..."));
	menu_append (tray_menu, CMD_ABOUT,    _("About"));
	AppendMenuW (tray_menu, MF_SEPARATOR, 0, NULL);

	// Section 5: Window, Offline, Quit
	menu_append (tray_menu, CMD_SHOW_WINDOW,  _("Show window"));
	menu_append (tray_menu, CMD_OFFLINE_MODE, _("Offline Mode"));
	menu_append (tray_menu, CMD_QUIT,         _("Quit"));
}

static void popup_menu (void)
{
	POINT pt;

	if (tray_menu == NULL)
		return;

	GetCursorPos (&pt);

	// SetForegroundWindow() before and the WM_NULL post after are the
	// documented workaround for a popup menu that would otherwise not
	// dismiss when the user clicks elsewhere.
	SetForegroundWindow (tray_hwnd);
	TrackPopupMenu (tray_menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, tray_hwnd, NULL);
	PostMessage (tray_hwnd, WM_NULL, 0, 0);
}

static void on_command (UgtkApp* app, UINT id)
{
	switch (id) {
	case CMD_NEW_DOWNLOAD:
		ugtk_app_create_download (app, NULL, NULL);
		break;

	case CMD_NEW_CLIPBOARD:
		ugtk_app_clipboard_batch (app);
		break;

	case CMD_NEW_TORRENT:
		ugtk_app_create_torrent (app);
		break;

	case CMD_NEW_METALINK:
		ugtk_app_create_metalink (app);
		break;

	case CMD_CLIPBOARD_MONITOR:
		app->setting.clipboard.monitor = toggle_menu_item (id);
		sync_action (app, "clipboard-monitor", app->setting.clipboard.monitor);
		break;

	case CMD_CLIPBOARD_QUIET:
		app->setting.clipboard.quiet = toggle_menu_item (id);
		sync_action (app, "clipboard-quiet", app->setting.clipboard.quiet);
		break;

	case CMD_COMMANDLINE_QUIET:
		app->setting.commandline.quiet = toggle_menu_item (id);
		sync_action (app, "commandline-quiet", app->setting.commandline.quiet);
		break;

	case CMD_SKIP_EXISTING:
		app->setting.ui.skip_existing = toggle_menu_item (id);
		sync_action (app, "skip-existing", app->setting.ui.skip_existing);
		break;

	case CMD_APPLY_RECENT:
		app->setting.ui.apply_recent = toggle_menu_item (id);
		sync_action (app, "apply-recent", app->setting.ui.apply_recent);
		break;

	case CMD_SETTINGS:
		activate_action (app, "settings");
		break;

	case CMD_ABOUT:
		activate_action (app, "about");
		break;

	case CMD_SHOW_WINDOW:
		show_main_window (app);
		break;

	case CMD_OFFLINE_MODE:
		app->setting.offline_mode = toggle_menu_item (id);
		sync_action (app, "offline-mode", app->setting.offline_mode);
		break;

	case CMD_QUIT:
		ugtk_app_decide_to_quit (app);
		break;

	default:
		break;
	}
}

// ----------------------------------------------------------------------------
// Window procedure

static LRESULT CALLBACK tray_wnd_proc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// Explorer restarted and dropped every tray icon; add ours back.
	if (msg == msg_taskbar_created && msg_taskbar_created != 0) {
		// Deliberately no NIM_SETVERSION: the default (version 0) keeps the
		// callback's lParam as the plain mouse message this WndProc expects.
		if (tray_added)
			Shell_NotifyIconW (NIM_ADD, &tray_nid);
		return 0;
	}

	switch (msg) {
	case UGTK_TRAY_MESSAGE:
		if (tray_app == NULL)
			break;
		switch (LOWORD (lparam)) {
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
			show_main_window (tray_app);
			break;

		case WM_RBUTTONUP:
		case WM_CONTEXTMENU:
			popup_menu ();
			break;

		default:
			break;
		}
		return 0;

	case WM_COMMAND:
		if (tray_app && HIWORD (wparam) == 0)
			on_command (tray_app, LOWORD (wparam));
		return 0;

	case WM_DESTROY:
		if (tray_added) {
			Shell_NotifyIconW (NIM_DELETE, &tray_nid);
			tray_added = FALSE;
		}
		return 0;

	default:
		break;
	}

	return DefWindowProcW (hwnd, msg, wparam, lparam);
}

// ----------------------------------------------------------------------------
// Public Functions

void ugtk_tray_icon_init (UgtkTrayIcon* trayicon, UgtkApp* app)
{
	WNDCLASSEXW wc;
	HINSTANCE   instance = GetModuleHandleW (NULL);

	trayicon->visible = FALSE;
	trayicon->error_occurred = FALSE;
	trayicon->state = UGTK_TRAY_ICON_STATE_NORMAL;

	tray_app = app;

	memset (&wc, 0, sizeof (wc));
	wc.cbSize        = sizeof (wc);
	wc.lpfnWndProc   = tray_wnd_proc;
	wc.hInstance     = instance;
	wc.lpszClassName = UGTK_TRAY_CLASS;
	RegisterClassExW (&wc);

	// A hidden ordinary window, deliberately not HWND_MESSAGE: message-only
	// windows do not receive the broadcast TaskbarCreated message, which is
	// how the icon survives an Explorer restart.
	tray_hwnd = CreateWindowExW (0, UGTK_TRAY_CLASS, UGTK_TRAY_CLASS,
	                             WS_OVERLAPPED, 0, 0, 0, 0,
	                             NULL, NULL, instance, NULL);
	if (tray_hwnd == NULL)
		return;

	msg_taskbar_created = RegisterWindowMessageW (L"TaskbarCreated");

	build_menu ();

	memset (&tray_nid, 0, sizeof (tray_nid));
	tray_nid.cbSize           = sizeof (tray_nid);
	tray_nid.hWnd             = tray_hwnd;
	tray_nid.uID              = 1;
	tray_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	tray_nid.uCallbackMessage = UGTK_TRAY_MESSAGE;
	tray_nid.hIcon            = load_state_icon (UGTK_TRAY_ICON_STATE_NORMAL);
	wcs_copy (tray_nid.szTip, G_N_ELEMENTS (tray_nid.szTip), UGTK_APP_NAME);
}

void ugtk_tray_icon_set_info (UgtkTrayIcon* trayicon, guint n_active, gint64 down_speed, gint64 up_speed)
{
	guint  current_state;
	gchar* down_str;
	gchar* up_str;
	gchar* tip;

	if (tray_hwnd == NULL)
		return;

	if (trayicon->error_occurred)
		current_state = UGTK_TRAY_ICON_STATE_ERROR;
	else if (n_active > 0)
		current_state = UGTK_TRAY_ICON_STATE_RUNNING;
	else
		current_state = UGTK_TRAY_ICON_STATE_NORMAL;

	if (trayicon->state != current_state) {
		HICON icon = load_state_icon (current_state);

		trayicon->state = current_state;
		trayicon->error_occurred = FALSE;

		if (icon) {
			if (tray_nid.hIcon)
				DestroyIcon (tray_nid.hIcon);
			tray_nid.hIcon = icon;
		}
	}

	// Unlike StatusNotifierItem, the shell shows a real tooltip here, so
	// spend it on the transfer rates.
	down_str = ug_str_from_int_unit (down_speed, "/s");
	up_str   = ug_str_from_int_unit (up_speed, "/s");
	// "Speed" and "Up Speed" are already in the message catalog (they label
	// the download list columns), so this needs no new translations.
	tip = g_strdup_printf ("%s\n" "%s: %s\n" "%s: %s",
	                       UGTK_APP_NAME,
	                       _("Speed"), down_str,
	                       _("Up Speed"), up_str);
	wcs_copy (tray_nid.szTip, G_N_ELEMENTS (tray_nid.szTip), tip);
	g_free (tip);
	ug_free (up_str);
	ug_free (down_str);

	if (tray_added) {
		tray_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		Shell_NotifyIconW (NIM_MODIFY, &tray_nid);
	}
}

void ugtk_tray_icon_set_visible (UgtkTrayIcon* trayicon, gboolean visible)
{
	trayicon->visible = visible;

	if (tray_hwnd == NULL)
		return;

	if (visible && !tray_added) {
		tray_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		if (Shell_NotifyIconW (NIM_ADD, &tray_nid))
			tray_added = TRUE;
	}
	else if (!visible && tray_added) {
		Shell_NotifyIconW (NIM_DELETE, &tray_nid);
		tray_added = FALSE;
	}
}

void ugtk_tray_icon_sync_menu (UgtkTrayIcon* trayicon, UgtkApp* app)
{
	if (tray_menu == NULL)
		return;

	CheckMenuItem (tray_menu, CMD_CLIPBOARD_MONITOR,
		MF_BYCOMMAND | (app->setting.clipboard.monitor ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (tray_menu, CMD_CLIPBOARD_QUIET,
		MF_BYCOMMAND | (app->setting.clipboard.quiet ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (tray_menu, CMD_COMMANDLINE_QUIET,
		MF_BYCOMMAND | (app->setting.commandline.quiet ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (tray_menu, CMD_SKIP_EXISTING,
		MF_BYCOMMAND | (app->setting.ui.skip_existing ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (tray_menu, CMD_APPLY_RECENT,
		MF_BYCOMMAND | (app->setting.ui.apply_recent ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (tray_menu, CMD_OFFLINE_MODE,
		MF_BYCOMMAND | (app->setting.offline_mode ? MF_CHECKED : MF_UNCHECKED));
}

void ugtk_tray_icon_set_plugin_sensitive (UgtkTrayIcon* trayicon, gboolean sensitive)
{
	UINT flags = MF_BYCOMMAND | (sensitive ? MF_ENABLED : MF_GRAYED);

	if (tray_menu == NULL)
		return;

	EnableMenuItem (tray_menu, CMD_NEW_TORRENT, flags);
	EnableMenuItem (tray_menu, CMD_NEW_METALINK, flags);
}

void ugtk_tray_icon_notify (UgtkTrayIcon* trayicon, const gchar* title, const gchar* body)
{
	gchar* full_title;

	// A balloon hangs off the tray icon, so there is nothing to show if the
	// icon is not currently in the notification area.
	if (tray_hwnd == NULL || !tray_added)
		return;

	full_title = g_strconcat (UGTK_APP_NAME " - ", title, NULL);
	wcs_copy (tray_nid.szInfoTitle, G_N_ELEMENTS (tray_nid.szInfoTitle), full_title);
	g_free (full_title);

	wcs_copy (tray_nid.szInfo, G_N_ELEMENTS (tray_nid.szInfo), body);

	tray_nid.uFlags     = NIF_INFO;
	tray_nid.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;   // uGet plays its own
	Shell_NotifyIconW (NIM_MODIFY, &tray_nid);

	// Leave the struct in its steady state for later NIM_MODIFY calls.
	tray_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
}
