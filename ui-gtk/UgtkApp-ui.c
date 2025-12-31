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

#include <UgtkApp.h>
#include <UgtkNodeList.h>
#include <UgtkNodeTree.h>
#include <UgtkNodeView.h>
#include <gdk/gdk.h>        // GdkScreen

#include <glib/gi18n.h>

static void ugtk_statusbar_init_ui (struct UgtkStatusbar* app_statusbar);
static void ugtk_toolbar_init_ui   (struct UgtkToolbar* ugt, gpointer accel_group);
static void ugtk_window_init_ui    (struct UgtkWindow* window, UgtkApp* app);
static void ugtk_app_init_size     (UgtkApp* app);
#if defined _WIN32 || defined _WIN64
static void ugtk_app_init_ui_win32 (UgtkApp* app, int screen_width);
#endif

void  ugtk_app_init_ui (UgtkApp* app)
{
	// Registers a new accelerator "Ctrl+N" with the global accelerator map.
	// Registers a new accelerator "Ctrl+N" with the global accelerator map.
	// gtk_accel_map_add_entry (UGTK_APP_ACCEL_PATH_NEW,      GDK_KEY_n,      GDK_CONTROL_MASK);
	// gtk_accel_map_add_entry (UGTK_APP_ACCEL_PATH_LOAD,     GDK_KEY_o,      GDK_CONTROL_MASK);
	// gtk_accel_map_add_entry (UGTK_APP_ACCEL_PATH_SAVE,     GDK_KEY_s,      GDK_CONTROL_MASK);
	// gtk_accel_map_add_entry (UGTK_APP_ACCEL_PATH_SAVE_ALL, GDK_KEY_s,      GDK_CONTROL_MASK | GDK_SHIFT_MASK);
	// gtk_accel_map_add_entry (UGTK_APP_ACCEL_PATH_DELETE,   GDK_KEY_Delete, 0);
//	// gtk_accel_map_add_entry (UGTK_APP_ACCEL_PATH_DELETE_F, GDK_KEY_Delete, GDK_SHIFT_MASK);
	// gtk_accel_map_add_entry (UGTK_APP_ACCEL_PATH_DELETE_F, GDK_KEY_Delete, GDK_CONTROL_MASK);
	// gtk_accel_map_add_entry (UGTK_APP_ACCEL_PATH_OPEN,     GDK_KEY_Return, 0);
	// gtk_accel_map_add_entry (UGTK_APP_ACCEL_PATH_OPEN_F,   GDK_KEY_Return, GDK_SHIFT_MASK);
	// NOTE: GtkAccelGroup is deprecated in GTK4. Keyboard shortcuts are not yet implemented.
	// See UgtkMenubar-ui.c for details on GTK4 migration requirements (GtkShortcutController).
	app->accel_group = NULL;
	// tray icon
	ugtk_tray_icon_init (&app->trayicon);
	// Main Window and it's widgets
	ugtk_banner_init (&app->banner);
	ugtk_menubar_init_ui (&app->menubar, app->accel_group);
	ugtk_summary_init (&app->summary, app->accel_group);
	ugtk_traveler_init (&app->traveler, app);
	ugtk_statusbar_init_ui (&app->statusbar);
	ugtk_toolbar_init_ui (&app->toolbar, NULL);
	ugtk_window_init_ui (&app->window, app);
	ugtk_app_init_size (app);
}

// set default size
// set default size
static void ugtk_app_init_size (UgtkApp* app)
{
	gint        width = 1000;
	gint        height = 700;
	gint        paned_position = 200;

	// decide window & traveler size
	// Hardcoded defaults for GTK4 port
	if (width <= 1000) {
		width = width * 90 / 100;
		height = height * 8 / 10;
		paned_position = 200;
	}

	gtk_window_set_default_size ((GtkWindow*) app->window.self, width, height);
	gtk_paned_set_position (app->window.hpaned, paned_position);

#if defined _WIN32 || defined _WIN64
	ugtk_app_init_ui_win32 (app, width);
#endif
}

#if defined _WIN32 || defined _WIN64
static void ugtk_app_init_ui_win32 (UgtkApp* app, int screen_width)
{
	GSettings*  gset;
	gint        sidebar_width;

#if 0
	// This will use icons\hicolor\index.theme
	GtkIconTheme*   icon_theme;
	gchar*          path;

	icon_theme = gtk_icon_theme_get_default ();
	path = g_build_filename (UG_DATADIR, "icons", NULL);
	gtk_icon_theme_append_search_path (icon_theme, path);
	g_free (path);
#endif

	if (screen_width <= 800)
		sidebar_width = 0;
	else if (screen_width <= 1200)
		sidebar_width = 180;
	else
		sidebar_width = 220;

	gset = g_settings_new ("org.gtk.Settings.FileChooser");
	g_settings_set_boolean (gset, "sort-directories-first", TRUE);

	// default of "sidebar-width" == 148
	if (sidebar_width > 0 && g_settings_get_int(gset, "sidebar-width") == 148)
		g_settings_set_int (gset, "sidebar-width", sidebar_width);
}
#endif  // _WIN32 || _WIN64

// ----------------------------------------------------------------------------
// UgtkWindow

static void ugtk_window_init_ui (struct UgtkWindow* window, UgtkApp* app)
{
	GtkBox*  vbox;
	GtkBox*  lbox;    // left side vbox
	GtkBox*  rbox;    // right side vbox

	window->self = (GtkWindow*) gtk_window_new ();
	gtk_window_set_title (window->self, UGTK_APP_NAME);
	// Explicitly request no custom titlebar to encourage native decoration
	gtk_window_set_titlebar (window->self, NULL);
//	gtk_window_resize (window->self, 640, 480);
	// GTK4: GtkAccelGroup removed, shortcuts not yet implemented
	gtk_window_set_default_icon_name (UGTK_APP_ICON_NAME);
#if GTK_MAJOR_VERSION <= 3 && GTK_MINOR_VERSION < 14
	gtk_window_set_has_resize_grip (window->self, FALSE);
#endif

	// top container for Main Window
	// top container for Main Window
	vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_window_set_child ((GtkWindow*) window->self, GTK_WIDGET (vbox));
	// banner + menubar
	gtk_box_append (vbox, app->banner.self);
	gtk_box_append (vbox, app->menubar.self);

	// hpaned
	window->hpaned = (GtkPaned*) gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_hexpand (GTK_WIDGET (window->hpaned), TRUE);
	gtk_widget_set_vexpand (GTK_WIDGET (window->hpaned), TRUE);
	gtk_box_append (vbox, GTK_WIDGET (window->hpaned));
	lbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	rbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_paned_set_start_child (window->hpaned, GTK_WIDGET (lbox));
	gtk_paned_set_end_child (window->hpaned, GTK_WIDGET (rbox));

	gtk_box_append (lbox, gtk_label_new (_("Status")));
	gtk_box_append (lbox, app->traveler.state.self);
	gtk_box_append (lbox, gtk_label_new (_("Category")));
	gtk_widget_set_hexpand (app->traveler.category.self, TRUE);
	gtk_widget_set_vexpand (app->traveler.category.self, TRUE);
	gtk_box_append (lbox, app->traveler.category.self);
	gtk_box_append (rbox, app->toolbar.self);

	// vpaned
	window->vpaned = (GtkPaned*) gtk_paned_new (GTK_ORIENTATION_VERTICAL);
	gtk_widget_set_hexpand ((GtkWidget*) window->vpaned, TRUE);
	gtk_widget_set_vexpand ((GtkWidget*) window->vpaned, TRUE);
	gtk_box_append (rbox, (GtkWidget*) window->vpaned);
	gtk_paned_set_start_child (window->vpaned, app->traveler.download.self);
	gtk_paned_set_end_child (window->vpaned, app->summary.self);

	gtk_box_append (vbox, GTK_WIDGET (app->statusbar.self));
	gtk_widget_set_visible ((GtkWidget*) vbox, TRUE);
}

// ----------------------------------------------------------------------------
// UgtkStatusbar
//
static void ugtk_statusbar_init_ui (struct UgtkStatusbar* sbar)
{
	GtkBox*    hbox;
	GtkWidget* widget;
	PangoContext*  context;
	PangoLayout*   layout;
	int            text_width;

	sbar->self = (GtkStatusbar*) gtk_statusbar_new ();
	hbox = GTK_BOX (sbar->self);

	// calculate text width
	context = gtk_widget_get_pango_context (GTK_WIDGET (sbar->self));
	layout = pango_layout_new (context);
	pango_layout_set_text (layout, "9999 MiB/s", -1);
	pango_layout_get_pixel_size (layout, &text_width, NULL);
	g_object_unref (layout);
	if (text_width < 100)
		text_width = 100;

	// upload speed label
	widget = gtk_label_new ("");
	sbar->up_speed = (GtkLabel*) widget;
	gtk_widget_set_size_request (widget, text_width, 0);
	// gtk_box_pack_end (hbox, widget, FALSE, TRUE, 2);
//	gtk_label_set_width_chars (sbar->down_speed, 15);
	// GTK4: Use gtk_label_set_xalign/yalign instead of deprecated gtk_misc_set_alignment
	gtk_label_set_xalign ((GtkLabel*)widget, 0.0);
	gtk_label_set_yalign ((GtkLabel*)widget, 0.5);
	// gtk_box_pack_end (hbox, gtk_label_new ("\xE2\x86\x91"), FALSE, TRUE, 2);    // "↑"

	// gtk_box_pack_end (hbox, gtk_separator_new (GTK_ORIENTATION_VERTICAL), FALSE, TRUE, 8);

	// download speed label
	widget = gtk_label_new ("");
	sbar->down_speed = (GtkLabel*) widget;
	gtk_widget_set_size_request (widget, text_width, 0);
	// gtk_box_pack_end (hbox, widget, FALSE, TRUE, 2);
//	gtk_label_set_width_chars (sbar->down_speed, 15);
	// GTK4: Use gtk_label_set_xalign/yalign instead of deprecated gtk_misc_set_alignment
	gtk_label_set_xalign ((GtkLabel*)widget, 0.0);
	gtk_label_set_yalign ((GtkLabel*)widget, 0.5);
	// gtk_box_pack_end (hbox, gtk_label_new ("\xE2\x86\x93"), FALSE, TRUE, 2);    // "↓"
}

// ----------------------------------------------------------------------------
// UgtkToolbar
//
static void ugtk_toolbar_init_ui (struct UgtkToolbar* ugt, gpointer accel_group)
{
	// GtkToolbar*   toolbar;
	// GtkToolItem*  tool_item;
	// GtkWidget*    image;
	// GtkWidget*    menu;
	// GtkWidget*    menu_item;

	// toolbar
	ugt->toolbar = (GtkWidget*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class (ugt->toolbar, "toolbar");
	ugt->self = (GtkWidget*) ugt->toolbar;
	gtk_widget_set_visible (ugt->self, TRUE);

	// New button (Menu Button)
	ugt->create = gtk_menu_button_new ();
    gtk_menu_button_set_icon_name (GTK_MENU_BUTTON (ugt->create), "document-new");
	gtk_widget_set_tooltip_text (ugt->create, _("Create new download"));
	gtk_box_append (GTK_BOX (ugt->toolbar), ugt->create);

    // Create Popover for "New" menu
    GtkWidget *popover = gtk_popover_new ();
    GtkWidget *menu_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_popover_set_child (GTK_POPOVER (popover), menu_box);
    gtk_menu_button_set_popover (GTK_MENU_BUTTON (ugt->create), popover);

	// Menu items (as flat buttons)
	ugt->create_download = gtk_button_new_with_label (_("New Download"));
    gtk_widget_add_css_class (ugt->create_download, "flat");
    gtk_box_append (GTK_BOX (menu_box), ugt->create_download);

	ugt->create_category = gtk_button_new_with_label (_("New Category"));
    gtk_widget_add_css_class (ugt->create_category, "flat");
    gtk_box_append (GTK_BOX (menu_box), ugt->create_category);

	ugt->create_sequence = gtk_button_new_with_label (_("New Batch Sequence"));
    gtk_widget_add_css_class (ugt->create_sequence, "flat");
    gtk_box_append (GTK_BOX (menu_box), ugt->create_sequence);

	ugt->create_clipboard = gtk_button_new_with_label (_("New Batch Clipboard"));
    gtk_widget_add_css_class (ugt->create_clipboard, "flat");
    gtk_box_append (GTK_BOX (menu_box), ugt->create_clipboard);
    
    // Separator?
    gtk_box_append (GTK_BOX (menu_box), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));

	ugt->create_torrent = gtk_button_new_with_label (_("New Torrent"));
    gtk_widget_add_css_class (ugt->create_torrent, "flat");
    gtk_box_append (GTK_BOX (menu_box), ugt->create_torrent);

	ugt->create_metalink = gtk_button_new_with_label (_("New Metalink"));
    gtk_widget_add_css_class (ugt->create_metalink, "flat");
    gtk_box_append (GTK_BOX (menu_box), ugt->create_metalink);

	// Save
	ugt->save = gtk_button_new_from_icon_name ("document-save");
	gtk_widget_set_tooltip_text (ugt->save, _("Save all settings"));
	gtk_box_append (GTK_BOX (ugt->toolbar), ugt->save);

	// Separator
	gtk_box_append (GTK_BOX (ugt->toolbar), gtk_separator_new (GTK_ORIENTATION_VERTICAL));

	// Runnable (Start)
	ugt->runnable = gtk_button_new_from_icon_name ("media-playback-start");
	gtk_widget_set_tooltip_text (ugt->runnable, _("Start"));
	gtk_box_append (GTK_BOX (ugt->toolbar), ugt->runnable);

	// Pause
	ugt->pause = gtk_button_new_from_icon_name ("media-playback-pause");
	gtk_widget_set_tooltip_text (ugt->pause, _("Pause"));
	gtk_box_append (GTK_BOX (ugt->toolbar), ugt->pause);

	// Properties
	ugt->properties = gtk_button_new_from_icon_name ("document-properties");
	gtk_widget_set_tooltip_text (ugt->properties, _("Properties"));
	gtk_box_append (GTK_BOX (ugt->toolbar), ugt->properties);

	// Separator
	gtk_box_append (GTK_BOX (ugt->toolbar), gtk_separator_new (GTK_ORIENTATION_VERTICAL));

	// Move Up
	ugt->move_up = gtk_button_new_from_icon_name ("go-up");
	gtk_widget_set_tooltip_text (ugt->move_up, _("Move Up"));
	gtk_box_append (GTK_BOX (ugt->toolbar), ugt->move_up);

	// Move Down
	ugt->move_down = gtk_button_new_from_icon_name ("go-down");
	gtk_widget_set_tooltip_text (ugt->move_down, _("Move Down"));
	gtk_box_append (GTK_BOX (ugt->toolbar), ugt->move_down);

	// Move Top
	ugt->move_top = gtk_button_new_from_icon_name ("go-top");
	gtk_widget_set_tooltip_text (ugt->move_top, _("Move Top"));
	gtk_box_append (GTK_BOX (ugt->toolbar), ugt->move_top);

	// Move Bottom
	ugt->move_bottom = gtk_button_new_from_icon_name ("go-bottom");
	gtk_widget_set_tooltip_text (ugt->move_bottom, _("Move Bottom"));
	gtk_box_append (GTK_BOX (ugt->toolbar), ugt->move_bottom);
}



