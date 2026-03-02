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
#include <UgtkApp.h>

#include <glib/gi18n.h>

#define UGTK_TRAY_ICON_NAME         "uget-tray-default"
#define UGTK_TRAY_ICON_ERROR_NAME   "uget-tray-error"
#define UGTK_TRAY_ICON_ACTIVE_NAME  "uget-tray-downloading"

#ifdef HAVE_APP_INDICATOR

// ============================================================================
// StatusNotifierItem D-Bus interface XML
// ============================================================================

static const gchar sni_introspection_xml[] =
	"<node>"
	"  <interface name='org.kde.StatusNotifierItem'>"
	"    <property name='Category' type='s' access='read'/>"
	"    <property name='Id' type='s' access='read'/>"
	"    <property name='Title' type='s' access='read'/>"
	"    <property name='Status' type='s' access='read'/>"
	"    <property name='IconName' type='s' access='read'/>"
	"    <property name='IconThemePath' type='s' access='read'/>"
	"    <property name='AttentionIconName' type='s' access='read'/>"
	"    <property name='Menu' type='o' access='read'/>"
	"    <property name='ItemIsMenu' type='b' access='read'/>"
	"    <property name='WindowId' type='u' access='read'/>"
	"    <property name='ToolTip' type='(sa(iiay)ss)' access='read'/>"
	"    <method name='ContextMenu'>"
	"      <arg direction='in' name='x' type='i'/>"
	"      <arg direction='in' name='y' type='i'/>"
	"    </method>"
	"    <method name='Activate'>"
	"      <arg direction='in' name='x' type='i'/>"
	"      <arg direction='in' name='y' type='i'/>"
	"    </method>"
	"    <method name='SecondaryActivate'>"
	"      <arg direction='in' name='x' type='i'/>"
	"      <arg direction='in' name='y' type='i'/>"
	"    </method>"
	"    <method name='Scroll'>"
	"      <arg direction='in' name='delta' type='i'/>"
	"      <arg direction='in' name='orientation' type='s'/>"
	"    </method>"
	"    <signal name='NewTitle'/>"
	"    <signal name='NewIcon'/>"
	"    <signal name='NewStatus'>"
	"      <arg type='s'/>"
	"    </signal>"
	"    <signal name='NewToolTip'/>"
	"  </interface>"
	"</node>";

// Store the app pointer for D-Bus method callbacks
static UgtkApp* sni_app = NULL;

// ============================================================================
// SNI D-Bus property/method handlers
// ============================================================================

static const gchar*
sni_status_string (UgtkTrayIcon* trayicon)
{
	if (!trayicon->visible)
		return "Passive";
	if (trayicon->state == UGTK_TRAY_ICON_STATE_ERROR ||
	    trayicon->state == UGTK_TRAY_ICON_STATE_RUNNING)
		return "NeedsAttention";
	return "Active";
}

static GVariant*
sni_get_property (GDBusConnection* connection,
                  const gchar* sender,
                  const gchar* object_path,
                  const gchar* interface_name,
                  const gchar* property_name,
                  GError** error,
                  gpointer user_data)
{
	UgtkTrayIcon* trayicon = (UgtkTrayIcon*) user_data;

	if (g_strcmp0 (property_name, "Category") == 0)
		return g_variant_new_string ("ApplicationStatus");
	if (g_strcmp0 (property_name, "Id") == 0)
		return g_variant_new_string ("uget-gtk");
	if (g_strcmp0 (property_name, "Title") == 0)
		return g_variant_new_string (UGTK_APP_NAME);
	if (g_strcmp0 (property_name, "Status") == 0)
		return g_variant_new_string (sni_status_string (trayicon));
	if (g_strcmp0 (property_name, "IconName") == 0)
		return g_variant_new_string (trayicon->icon_name ? trayicon->icon_name : UGTK_TRAY_ICON_NAME);
	if (g_strcmp0 (property_name, "IconThemePath") == 0)
		return g_variant_new_string (trayicon->icon_theme_path ? trayicon->icon_theme_path : "");
	if (g_strcmp0 (property_name, "AttentionIconName") == 0)
		return g_variant_new_string (trayicon->attention_icon_name ? trayicon->attention_icon_name : UGTK_TRAY_ICON_ACTIVE_NAME);
	if (g_strcmp0 (property_name, "Menu") == 0)
		return g_variant_new_object_path ("/MenuBar");
	if (g_strcmp0 (property_name, "ItemIsMenu") == 0)
		return g_variant_new_boolean (FALSE);
	if (g_strcmp0 (property_name, "WindowId") == 0)
		return g_variant_new_uint32 (0);
	if (g_strcmp0 (property_name, "ToolTip") == 0) {
		// (sa(iiay)ss) - icon-name, icon-pixmap array, title, description
		GVariantBuilder icon_array;
		g_variant_builder_init (&icon_array, G_VARIANT_TYPE ("a(iiay)"));
		return g_variant_new ("(sa(iiay)ss)", "", &icon_array,
		                      UGTK_APP_NAME, "");
	}

	g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY,
	             "Unknown property: %s", property_name);
	return NULL;
}

static void
sni_method_call (GDBusConnection* connection,
                 const gchar* sender,
                 const gchar* object_path,
                 const gchar* interface_name,
                 const gchar* method_name,
                 GVariant* parameters,
                 GDBusMethodInvocation* invocation,
                 gpointer user_data)
{
	if (g_strcmp0 (method_name, "Activate") == 0) {
		// Left click: show/present window
		if (sni_app) {
			if (gtk_widget_get_visible ((GtkWidget*) sni_app->window.self))
				gtk_window_present (sni_app->window.self);
			else {
				gtk_widget_set_visible ((GtkWidget*) sni_app->window.self, TRUE);
				gtk_window_present (sni_app->window.self);
				ugtk_app_decide_trayicon_visible (sni_app);
			}
		}
	}
	// ContextMenu, SecondaryActivate, Scroll: no-op (menu handled by dbusmenu)
	g_dbus_method_invocation_return_value (invocation, NULL);
}

static const GDBusInterfaceVTable sni_vtable = {
	sni_method_call,
	sni_get_property,
	NULL,  // set_property not needed
};

// ============================================================================
// SNI D-Bus signal helpers
// ============================================================================

static void
sni_emit_signal (UgtkTrayIcon* trayicon, const gchar* signal_name, GVariant* params)
{
	if (!trayicon->connection)
		return;
	g_dbus_connection_emit_signal (trayicon->connection, NULL,
	                               "/StatusNotifierItem",
	                               "org.kde.StatusNotifierItem",
	                               signal_name, params, NULL);
}

static void
sni_emit_new_icon (UgtkTrayIcon* trayicon)
{
	sni_emit_signal (trayicon, "NewIcon", NULL);
}

static void
sni_emit_new_status (UgtkTrayIcon* trayicon)
{
	sni_emit_signal (trayicon, "NewStatus",
	                 g_variant_new ("(s)", sni_status_string (trayicon)));
}

static void
sni_emit_new_tooltip (UgtkTrayIcon* trayicon)
{
	sni_emit_signal (trayicon, "NewToolTip", NULL);
}

// ============================================================================
// Register with StatusNotifierWatcher
// ============================================================================

static void
sni_register_with_watcher (UgtkTrayIcon* trayicon)
{
	if (!trayicon->connection)
		return;

	gchar* bus_name = g_strdup_printf ("org.kde.StatusNotifierItem-%d-1", getpid ());
	g_dbus_connection_call (trayicon->connection,
	                        "org.kde.StatusNotifierWatcher",
	                        "/StatusNotifierWatcher",
	                        "org.kde.StatusNotifierWatcher",
	                        "RegisterStatusNotifierItem",
	                        g_variant_new ("(s)", bus_name),
	                        NULL,
	                        G_DBUS_CALL_FLAGS_NONE,
	                        -1, NULL, NULL, NULL);
	g_free (bus_name);
}

// ============================================================================
// Bus name acquired/lost callbacks
// ============================================================================

static void
on_bus_name_acquired (GDBusConnection* connection, const gchar* name, gpointer user_data)
{
	UgtkTrayIcon* trayicon = (UgtkTrayIcon*) user_data;
	sni_register_with_watcher (trayicon);
}

static void
on_bus_name_lost (GDBusConnection* connection, const gchar* name, gpointer user_data)
{
	// Connection lost or name couldn't be acquired
}

// ============================================================================
// Menu item callbacks
// ============================================================================

static void
on_create_download (DbusmenuMenuitem* item, guint timestamp, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	ugtk_app_create_download (app, NULL, NULL);
}

static void
on_create_clipboard (DbusmenuMenuitem* item, guint timestamp, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	ugtk_app_clipboard_batch (app);
}

static void
on_create_torrent (DbusmenuMenuitem* item, guint timestamp, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	ugtk_app_create_torrent (app);
}

static void
on_create_metalink (DbusmenuMenuitem* item, guint timestamp, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	ugtk_app_create_metalink (app);
}

static void
on_toggle_item (DbusmenuMenuitem* item, guint timestamp, gpointer user_data)
{
	int current = dbusmenu_menuitem_property_get_int (item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE);
	int new_state = (current == DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED)
		? DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED
		: DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED;
	dbusmenu_menuitem_property_set_int (item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE, new_state);
}

static void
on_clipboard_monitor (DbusmenuMenuitem* item, guint timestamp, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	on_toggle_item (item, timestamp, user_data);
	gboolean active = dbusmenu_menuitem_property_get_int (item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE)
		== DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED;
	app->setting.clipboard.monitor = active;
	GAction* menubar_action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "clipboard-monitor");
	if (menubar_action)
		g_action_change_state (menubar_action, g_variant_new_boolean (active));
}

static void
on_clipboard_quiet (DbusmenuMenuitem* item, guint timestamp, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	on_toggle_item (item, timestamp, user_data);
	gboolean active = dbusmenu_menuitem_property_get_int (item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE)
		== DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED;
	app->setting.clipboard.quiet = active;
	GAction* menubar_action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "clipboard-quiet");
	if (menubar_action)
		g_action_change_state (menubar_action, g_variant_new_boolean (active));
}

static void
on_commandline_quiet (DbusmenuMenuitem* item, guint timestamp, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	on_toggle_item (item, timestamp, user_data);
	gboolean active = dbusmenu_menuitem_property_get_int (item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE)
		== DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED;
	app->setting.commandline.quiet = active;
	GAction* menubar_action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "commandline-quiet");
	if (menubar_action)
		g_action_change_state (menubar_action, g_variant_new_boolean (active));
}

static void
on_skip_existing (DbusmenuMenuitem* item, guint timestamp, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	on_toggle_item (item, timestamp, user_data);
	gboolean active = dbusmenu_menuitem_property_get_int (item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE)
		== DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED;
	app->setting.ui.skip_existing = active;
	GAction* menubar_action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "skip-existing");
	if (menubar_action)
		g_action_change_state (menubar_action, g_variant_new_boolean (active));
}

static void
on_apply_recent (DbusmenuMenuitem* item, guint timestamp, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	on_toggle_item (item, timestamp, user_data);
	gboolean active = dbusmenu_menuitem_property_get_int (item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE)
		== DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED;
	app->setting.ui.apply_recent = active;
	GAction* menubar_action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "apply-recent");
	if (menubar_action)
		g_action_change_state (menubar_action, g_variant_new_boolean (active));
}

static void
on_settings (DbusmenuMenuitem* item, guint timestamp, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	GAction* menubar_action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "settings");
	if (menubar_action)
		g_action_activate (menubar_action, NULL);
}

static void
on_about (DbusmenuMenuitem* item, guint timestamp, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	GAction* menubar_action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "about");
	if (menubar_action)
		g_action_activate (menubar_action, NULL);
}

static void
on_show_window (DbusmenuMenuitem* item, guint timestamp, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	if (gtk_widget_get_visible ((GtkWidget*) app->window.self))
		gtk_window_present (app->window.self);
	else {
		gtk_widget_set_visible ((GtkWidget*) app->window.self, TRUE);
		gtk_window_present (app->window.self);
		ugtk_app_decide_trayicon_visible (app);
	}
}

static void
on_offline_mode (DbusmenuMenuitem* item, guint timestamp, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	on_toggle_item (item, timestamp, user_data);
	gboolean active = dbusmenu_menuitem_property_get_int (item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE)
		== DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED;
	app->setting.offline_mode = active;
	GAction* menubar_action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "offline-mode");
	if (menubar_action)
		g_action_change_state (menubar_action, g_variant_new_boolean (active));
}

static void
on_quit (DbusmenuMenuitem* item, guint timestamp, gpointer user_data)
{
	UgtkApp* app = (UgtkApp*) user_data;
	ugtk_app_decide_to_quit (app);
}

// ============================================================================
// Menu construction helpers
// ============================================================================

static DbusmenuMenuitem*
create_menu_item (const gchar* label, GCallback callback, gpointer user_data)
{
	DbusmenuMenuitem* item = dbusmenu_menuitem_new ();
	dbusmenu_menuitem_property_set (item, DBUSMENU_MENUITEM_PROP_LABEL, label);
	g_signal_connect (item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, callback, user_data);
	return item;
}

static DbusmenuMenuitem*
create_toggle_item (const gchar* label, gboolean initial_state, GCallback callback, gpointer user_data)
{
	DbusmenuMenuitem* item = dbusmenu_menuitem_new ();
	dbusmenu_menuitem_property_set (item, DBUSMENU_MENUITEM_PROP_LABEL, label);
	dbusmenu_menuitem_property_set (item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_CHECK);
	dbusmenu_menuitem_property_set_int (item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
		initial_state ? DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED : DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);
	g_signal_connect (item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, callback, user_data);
	return item;
}

static DbusmenuMenuitem*
create_separator (void)
{
	DbusmenuMenuitem* item = dbusmenu_menuitem_new ();
	dbusmenu_menuitem_property_set (item, DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_CLIENT_TYPES_SEPARATOR);
	return item;
}

static void
build_menu (UgtkTrayIcon* trayicon, UgtkApp* app)
{
	DbusmenuMenuitem* root = dbusmenu_menuitem_new ();
	trayicon->root_menuitem = root;

	// Section 1: Create downloads
	dbusmenu_menuitem_child_append (root,
		create_menu_item (_("New Download..."), G_CALLBACK (on_create_download), app));
	dbusmenu_menuitem_child_append (root,
		create_menu_item (_("New Clipboard batch..."), G_CALLBACK (on_create_clipboard), app));

	dbusmenu_menuitem_child_append (root, create_separator ());

	// Section 2: Torrent/Metalink
	trayicon->item_create_torrent = create_menu_item (_("New Torrent..."), G_CALLBACK (on_create_torrent), app);
	dbusmenu_menuitem_child_append (root, trayicon->item_create_torrent);
	trayicon->item_create_metalink = create_menu_item (_("New Metalink..."), G_CALLBACK (on_create_metalink), app);
	dbusmenu_menuitem_child_append (root, trayicon->item_create_metalink);

	dbusmenu_menuitem_child_append (root, create_separator ());

	// Section 3: Setting toggles
	trayicon->item_clipboard_monitor = create_toggle_item (
		_("Clipboard Monitor"), FALSE, G_CALLBACK (on_clipboard_monitor), app);
	dbusmenu_menuitem_child_append (root, trayicon->item_clipboard_monitor);

	trayicon->item_clipboard_quiet = create_toggle_item (
		_("Clipboard works quietly"), FALSE, G_CALLBACK (on_clipboard_quiet), app);
	dbusmenu_menuitem_child_append (root, trayicon->item_clipboard_quiet);

	trayicon->item_commandline_quiet = create_toggle_item (
		_("Command-line works quietly"), FALSE, G_CALLBACK (on_commandline_quiet), app);
	dbusmenu_menuitem_child_append (root, trayicon->item_commandline_quiet);

	trayicon->item_skip_existing = create_toggle_item (
		_("Skip existing URI"), FALSE, G_CALLBACK (on_skip_existing), app);
	dbusmenu_menuitem_child_append (root, trayicon->item_skip_existing);

	trayicon->item_apply_recent = create_toggle_item (
		_("Apply recent download settings"), FALSE, G_CALLBACK (on_apply_recent), app);
	dbusmenu_menuitem_child_append (root, trayicon->item_apply_recent);

	dbusmenu_menuitem_child_append (root, create_separator ());

	// Section 4: Settings and About
	dbusmenu_menuitem_child_append (root,
		create_menu_item (_("Settings..."), G_CALLBACK (on_settings), app));
	dbusmenu_menuitem_child_append (root,
		create_menu_item (_("About"), G_CALLBACK (on_about), app));

	dbusmenu_menuitem_child_append (root, create_separator ());

	// Section 5: Window, Offline, Quit
	dbusmenu_menuitem_child_append (root,
		create_menu_item (_("Show window"), G_CALLBACK (on_show_window), app));
	trayicon->item_offline_mode = create_toggle_item (
		_("Offline Mode"), FALSE, G_CALLBACK (on_offline_mode), app);
	dbusmenu_menuitem_child_append (root, trayicon->item_offline_mode);
	dbusmenu_menuitem_child_append (root,
		create_menu_item (_("Quit"), G_CALLBACK (on_quit), app));

	// Attach root to DbusmenuServer
	trayicon->menu_server = dbusmenu_server_new ("/MenuBar");
	dbusmenu_server_set_root (trayicon->menu_server, root);
}

#endif // HAVE_APP_INDICATOR

// ============================================================================
// Public Functions
// ============================================================================

void ugtk_tray_icon_init (UgtkTrayIcon* trayicon, UgtkApp* app)
{
	trayicon->visible = FALSE;
	trayicon->error_occurred = FALSE;
	trayicon->state = UGTK_TRAY_ICON_STATE_NORMAL;

#ifdef HAVE_APP_INDICATOR
	GError* error = NULL;
	GDBusNodeInfo* node_info = NULL;

	sni_app = app;

	// Determine icon theme path for development fallback
	// IconThemePath must point to the directory CONTAINING "hicolor/",
	// not the "hicolor/" directory itself, because the icon theme system
	// expects to find theme directories (like hicolor/) within the search path.
	trayicon->icon_theme_path = NULL;
	gchar* file_name = g_build_filename (UG_DATADIR, "icons",
	                                     "hicolor", "24x24", "apps",
	                                     "uget-tray-default.png", NULL);
	if (!g_file_test (file_name, G_FILE_TEST_IS_REGULAR)) {
		gchar* cwd = g_get_current_dir ();
		gchar* source_icons = g_build_filename (cwd, "pixmaps", "icons", NULL);
		gchar* source_hicolor = g_build_filename (source_icons, "hicolor", NULL);
		if (!g_file_test (source_hicolor, G_FILE_TEST_IS_DIR)) {
			g_free (source_icons);
			g_free (source_hicolor);
			gchar* parent = g_path_get_dirname (cwd);
			source_icons = g_build_filename (parent, "pixmaps", "icons", NULL);
			source_hicolor = g_build_filename (source_icons, "hicolor", NULL);
			g_free (parent);
		}
		if (g_file_test (source_hicolor, G_FILE_TEST_IS_DIR))
			trayicon->icon_theme_path = source_icons;
		else
			g_free (source_icons);
		g_free (source_hicolor);
		g_free (cwd);
	}
	g_free (file_name);

	trayicon->icon_name = g_strdup (UGTK_TRAY_ICON_NAME);
	trayicon->attention_icon_name = g_strdup (UGTK_TRAY_ICON_ACTIVE_NAME);

	// Connect to session bus
	trayicon->connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
	if (!trayicon->connection) {
		g_warning ("Failed to connect to session bus: %s", error->message);
		g_error_free (error);
		return;
	}

	// Parse introspection XML and register the object
	node_info = g_dbus_node_info_new_for_xml (sni_introspection_xml, &error);
	if (!node_info) {
		g_warning ("Failed to parse SNI introspection XML: %s", error->message);
		g_error_free (error);
		return;
	}

	trayicon->registration_id = g_dbus_connection_register_object (
		trayicon->connection,
		"/StatusNotifierItem",
		node_info->interfaces[0],
		&sni_vtable,
		trayicon,
		NULL,
		&error);
	g_dbus_node_info_unref (node_info);

	if (trayicon->registration_id == 0) {
		g_warning ("Failed to register SNI object: %s", error->message);
		g_error_free (error);
		return;
	}

	// Build the dbusmenu
	build_menu (trayicon, app);

	// Acquire the well-known bus name
	gchar* bus_name = g_strdup_printf ("org.kde.StatusNotifierItem-%d-1", getpid ());
	trayicon->bus_name_id = g_bus_own_name (G_BUS_TYPE_SESSION,
	                                         bus_name,
	                                         G_BUS_NAME_OWNER_FLAGS_NONE,
	                                         NULL,
	                                         on_bus_name_acquired,
	                                         on_bus_name_lost,
	                                         trayicon,
	                                         NULL);
	g_free (bus_name);
#endif
}

void ugtk_tray_icon_set_info (UgtkTrayIcon* trayicon, guint n_active, gint64 down_speed, gint64 up_speed)
{
#ifdef HAVE_APP_INDICATOR
	if (!trayicon->connection)
		return;

	const gchar* new_icon;
	guint        current_state;

	// Determine icon state
	if (trayicon->error_occurred) {
		new_icon = UGTK_TRAY_ICON_ERROR_NAME;
		current_state = UGTK_TRAY_ICON_STATE_ERROR;
	}
	else if (n_active > 0) {
		new_icon = UGTK_TRAY_ICON_ACTIVE_NAME;
		current_state = UGTK_TRAY_ICON_STATE_RUNNING;
	}
	else {
		new_icon = UGTK_TRAY_ICON_NAME;
		current_state = UGTK_TRAY_ICON_STATE_NORMAL;
	}

	// Update icon if state changed
	if (trayicon->state != current_state) {
		trayicon->state = current_state;
		trayicon->error_occurred = FALSE;

		g_free (trayicon->icon_name);
		trayicon->icon_name = g_strdup (new_icon);

		if (trayicon->visible) {
			sni_emit_new_icon (trayicon);
			sni_emit_new_status (trayicon);
		}
	}

	// Emit tooltip update
	sni_emit_new_tooltip (trayicon);
#endif
}

void ugtk_tray_icon_set_visible (UgtkTrayIcon* trayicon, gboolean visible)
{
	trayicon->visible = visible;

#ifdef HAVE_APP_INDICATOR
	if (!trayicon->connection)
		return;

	sni_emit_new_status (trayicon);
#endif
}

void ugtk_tray_icon_sync_menu (UgtkTrayIcon* trayicon, UgtkApp* app)
{
#ifdef HAVE_APP_INDICATOR
	if (!trayicon->root_menuitem)
		return;

	dbusmenu_menuitem_property_set_int (trayicon->item_clipboard_monitor,
		DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
		app->setting.clipboard.monitor ? DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED : DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);

	dbusmenu_menuitem_property_set_int (trayicon->item_clipboard_quiet,
		DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
		app->setting.clipboard.quiet ? DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED : DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);

	dbusmenu_menuitem_property_set_int (trayicon->item_commandline_quiet,
		DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
		app->setting.commandline.quiet ? DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED : DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);

	dbusmenu_menuitem_property_set_int (trayicon->item_skip_existing,
		DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
		app->setting.ui.skip_existing ? DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED : DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);

	dbusmenu_menuitem_property_set_int (trayicon->item_apply_recent,
		DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
		app->setting.ui.apply_recent ? DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED : DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);

	dbusmenu_menuitem_property_set_int (trayicon->item_offline_mode,
		DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
		app->setting.offline_mode ? DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED : DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);
#endif
}

void ugtk_tray_icon_set_plugin_sensitive (UgtkTrayIcon* trayicon, gboolean sensitive)
{
#ifdef HAVE_APP_INDICATOR
	if (!trayicon->root_menuitem)
		return;

	dbusmenu_menuitem_property_set_bool (trayicon->item_create_torrent,
		DBUSMENU_MENUITEM_PROP_ENABLED, sensitive);
	dbusmenu_menuitem_property_set_bool (trayicon->item_create_metalink,
		DBUSMENU_MENUITEM_PROP_ENABLED, sensitive);
#endif
}
