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

#include <gtk/gtk.h>
#include <UgtkConfig.h>

#ifdef HAVE_APP_INDICATOR
#include <libappindicator/app-indicator.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UgtkTrayIcon   UgtkTrayIcon;
typedef struct UgtkApp        UgtkApp;
// --------------------------------
// Tray Icon

enum UgtkTrayIconState
{
	UGTK_TRAY_ICON_STATE_NORMAL,
	UGTK_TRAY_ICON_STATE_RUNNING,
	UGTK_TRAY_ICON_STATE_ERROR,
};

struct UgtkTrayIcon
{
#ifdef HAVE_APP_INDICATOR
	AppIndicator*  indicator;
	AppIndicator*  indicator_temp;
#endif
	// GtkStatusIcon* self;
	gboolean       visible;
	gboolean       error_occurred;
	guint          state;    // UgtkTrayIconState

	struct UgtkTrayIconMenu
	{
		GtkWidget*  self;    // (GtkMenu) pop-up menu

		GtkWidget*  create_download;
		GtkWidget*  create_clipboard;
		GtkWidget*  create_torrent;
		GtkWidget*  create_metalink;

		gboolean    emission;
		GtkWidget*  clipboard_monitor;
		GtkWidget*  clipboard_quiet;
		GtkWidget*  commandline_quiet;
		GtkWidget*  skip_existing;
		GtkWidget*  apply_recent;

		GtkWidget*  settings;
		GtkWidget*  about;
		GtkWidget*  show_window;
		GtkWidget*  offline_mode;
		GtkWidget*  quit;
	} menu;
};

void  ugtk_tray_icon_init (UgtkTrayIcon* trayicon);
void  ugtk_tray_icon_set_info (UgtkTrayIcon* trayicon, guint n_active, gint64 down_speed, gint64 up_speed);
void  ugtk_tray_icon_set_visible (UgtkTrayIcon* trayicon, gboolean visible);

#ifdef HAVE_APP_INDICATOR
void  ugtk_tray_icon_use_indicator (UgtkTrayIcon* trayicon, gboolean enable);
#endif

void  ugtk_trayicon_init_callback (UgtkTrayIcon* trayicon, UgtkApp* app);

#ifdef __cplusplus
}
#endif

#endif // UGTK_TRAY_H
