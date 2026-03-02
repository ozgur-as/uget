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

#ifndef UGTK_TRAY_ICON_H
#define UGTK_TRAY_ICON_H

#include <gio/gio.h>
#include <UgtkConfig.h>

#ifdef HAVE_APP_INDICATOR
#include <libdbusmenu-glib/server.h>
#include <libdbusmenu-glib/menuitem.h>
#include <libdbusmenu-glib/client.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UgtkTrayIcon   UgtkTrayIcon;
typedef struct UgtkApp        UgtkApp;

// --------------------------------
// Tray Icon State

enum UgtkTrayIconState
{
	UGTK_TRAY_ICON_STATE_NORMAL,
	UGTK_TRAY_ICON_STATE_RUNNING,
	UGTK_TRAY_ICON_STATE_ERROR,
};

// --------------------------------
// Tray Icon Structure

struct UgtkTrayIcon
{
#ifdef HAVE_APP_INDICATOR
	GDBusConnection*    connection;
	guint               bus_name_id;
	guint               registration_id;
	DbusmenuServer*     menu_server;
	DbusmenuMenuitem*   root_menuitem;
	// Menu items we need references to for state sync
	DbusmenuMenuitem*   item_clipboard_monitor;
	DbusmenuMenuitem*   item_clipboard_quiet;
	DbusmenuMenuitem*   item_commandline_quiet;
	DbusmenuMenuitem*   item_skip_existing;
	DbusmenuMenuitem*   item_apply_recent;
	DbusmenuMenuitem*   item_offline_mode;
	DbusmenuMenuitem*   item_create_torrent;
	DbusmenuMenuitem*   item_create_metalink;
	gchar*              icon_name;
	gchar*              attention_icon_name;
	gchar*              icon_theme_path;
#endif
	gboolean            visible;
	gboolean            error_occurred;
	guint               state;    // UgtkTrayIconState
};

void  ugtk_tray_icon_init (UgtkTrayIcon* trayicon, UgtkApp* app);
void  ugtk_tray_icon_set_info (UgtkTrayIcon* trayicon, guint n_active, gint64 down_speed, gint64 up_speed);
void  ugtk_tray_icon_set_visible (UgtkTrayIcon* trayicon, gboolean visible);

// Sync menu checkbox states with app settings
void  ugtk_tray_icon_sync_menu (UgtkTrayIcon* trayicon, UgtkApp* app);

// Set sensitivity of torrent/metalink menu items
void  ugtk_tray_icon_set_plugin_sensitive (UgtkTrayIcon* trayicon, gboolean sensitive);

#ifdef __cplusplus
}
#endif

#endif // UGTK_TRAY_ICON_H
