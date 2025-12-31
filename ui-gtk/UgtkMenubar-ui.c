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

#include <UgtkMenubar.h>
#include <UgtkApp.h>
#include <UgtkAboutDialog.h>
#include <UgtkNodeView.h>
#include <UgtkSummary.h>

#if defined _WIN32 || defined _WIN64
#include <windows.h>
#include <shellapi.h>
#endif

#include <glib/gi18n.h>

/*
 * GTK4 PORT NOTE:
 * This entire file originally contained menu creation functions using GTK3 APIs that were removed in GTK4:
 * - GtkMenu, GtkMenuItem, GtkImageMenuItem, GtkCheckMenuItem (all removed)  
 * - GTK_STOCK_* constants (all removed)
 * - GtkAccelGroup (removed)
 * - gtk_menu_bar_new, gtk_menu_new, gtk_menu_item_new_from_stock, etc. (all removed)
 *  
 * All menu creation code has been stubbed out. The application will compile but will not have a functional
 * menubar. To restore menu functionality, this file needs a complete rewrite using:
 * - GMenu for the menu model
 * - GtkPopoverMenu or GtkMenuButton for menu presentation
 */

void ugtk_menubar_init_ui (UgtkMenubar* menubar, void* accel_group)
{
    GMenu *menu_model = g_menu_new ();
    GMenu *submenu;
    GMenu *section;
    GMenu *batch_submenu;

    // =========================================================================
    // --- File Menu ---
    // =========================================================================
    submenu = g_menu_new ();
    
    // New Download / New Category
    g_menu_append (submenu, _("New Download"), "win.new-download");
    g_menu_append (submenu, _("New Category"), "win.new-category");
    
    // Batch Downloads (submenu)
    section = g_menu_new ();
    batch_submenu = g_menu_new ();
    g_menu_append (batch_submenu, _("Clipboard batch"), "win.batch-clipboard");
    g_menu_append (batch_submenu, _("URL Sequence batch"), "win.batch-sequence");
    g_menu_append (batch_submenu, _("Text file import (.txt)"), "win.import-text");
    g_menu_append (batch_submenu, _("HTML file import (.html)"), "win.import-html");
    g_menu_append (batch_submenu, _("Export to Text File (.txt)"), "win.export-text");
    g_menu_append_submenu (section, _("Batch Downloads"), G_MENU_MODEL (batch_submenu));
    g_object_unref (batch_submenu);
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);

    // Open / Save Category / Save settings
    section = g_menu_new ();
    g_menu_append (section, _("Open category"), "win.open-category");
    g_menu_append (section, _("Save category as"), "win.save-category");
    g_menu_append (section, _("Save all settings"), "win.save");
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);

    // Offline Mode
    section = g_menu_new ();
    g_menu_append (section, _("Offline Mode"), "win.offline-mode");
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);

    // Quit
    section = g_menu_new ();
    g_menu_append (section, _("Quit"), "win.quit");
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);

    g_menu_append_submenu (menu_model, _("File"), G_MENU_MODEL (submenu));
    g_object_unref (submenu);

    // =========================================================================
    // --- Edit Menu ---
    // =========================================================================
    submenu = g_menu_new ();
    
    // Clipboard options, skip/apply
    g_menu_append (submenu, _("Clipboard Monitor"), "win.clipboard-monitor");
    g_menu_append (submenu, _("Clipboard works quietly"), "win.clipboard-quiet");
    g_menu_append (submenu, _("Command-line works quietly"), "win.commandline-quiet");
    g_menu_append (submenu, _("Skip existing URI"), "win.skip-existing");
    g_menu_append (submenu, _("Apply recent download settings"), "win.apply-recent");

    // Completion Auto-Actions (submenu) - radio-style selection
    section = g_menu_new ();
    GMenu *completion_submenu = g_menu_new ();
    // Use g_menu_item_new with target to create radio items
    GMenuItem *item;
    item = g_menu_item_new (_("Disable"), NULL);
    g_menu_item_set_action_and_target (item, "win.completion-action", "s", "disable");
    g_menu_append_item (completion_submenu, item);
    g_object_unref (item);
    item = g_menu_item_new (_("Hibernate"), NULL);
    g_menu_item_set_action_and_target (item, "win.completion-action", "s", "hibernate");
    g_menu_append_item (completion_submenu, item);
    g_object_unref (item);
    item = g_menu_item_new (_("Suspend"), NULL);
    g_menu_item_set_action_and_target (item, "win.completion-action", "s", "suspend");
    g_menu_append_item (completion_submenu, item);
    g_object_unref (item);
    item = g_menu_item_new (_("Shutdown"), NULL);
    g_menu_item_set_action_and_target (item, "win.completion-action", "s", "shutdown");
    g_menu_append_item (completion_submenu, item);
    g_object_unref (item);
    item = g_menu_item_new (_("Reboot"), NULL);
    g_menu_item_set_action_and_target (item, "win.completion-action", "s", "reboot");
    g_menu_append_item (completion_submenu, item);
    g_object_unref (item);
    item = g_menu_item_new (_("Custom"), NULL);
    g_menu_item_set_action_and_target (item, "win.completion-action", "s", "custom");
    g_menu_append_item (completion_submenu, item);
    g_object_unref (item);
    GMenu *comp_section = g_menu_new ();
    g_menu_append (comp_section, _("Remember"), "win.completion-remember");
    g_menu_append (comp_section, _("Help"), "win.completion-help");
    g_menu_append_section (completion_submenu, NULL, G_MENU_MODEL (comp_section));
    g_object_unref (comp_section);
    g_menu_append_submenu (section, _("Completion Auto-Actions"), G_MENU_MODEL (completion_submenu));
    g_object_unref (completion_submenu);
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);

    // Settings
    section = g_menu_new ();
    g_menu_append (section, _("Settings"), "win.settings");
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);

    g_menu_append_submenu (menu_model, _("Edit"), G_MENU_MODEL (submenu));
    g_object_unref (submenu);

    // =========================================================================
    // --- View Menu ---
    // =========================================================================
    submenu = g_menu_new ();
    g_menu_append (submenu, _("Toolbar"), "win.view-toolbar");
    g_menu_append (submenu, _("Statusbar"), "win.view-statusbar");
    g_menu_append (submenu, _("Category"), "win.view-category");
    g_menu_append (submenu, _("Summary"), "win.view-summary");

    // Summary Items (submenu)
    section = g_menu_new ();
    GMenu *summary_submenu = g_menu_new ();
    g_menu_append (summary_submenu, _("Name"), "win.summary-name");
    g_menu_append (summary_submenu, _("Folder"), "win.summary-folder");
    g_menu_append (summary_submenu, _("Category"), "win.summary-category");
    g_menu_append (summary_submenu, _("URI"), "win.summary-uri");
    g_menu_append (summary_submenu, _("Message"), "win.summary-message");
    g_menu_append_submenu (section, _("Summary Items"), G_MENU_MODEL (summary_submenu));
    g_object_unref (summary_submenu);
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);

    // Download Columns (submenu)
    section = g_menu_new ();
    GMenu *columns_submenu = g_menu_new ();
    g_menu_append (columns_submenu, _("Complete"), "win.column-complete");
    g_menu_append (columns_submenu, _("Total"), "win.column-total");
    g_menu_append (columns_submenu, _("Percent"), "win.column-percent");
    g_menu_append (columns_submenu, _("Elapsed"), "win.column-elapsed");
    g_menu_append (columns_submenu, _("Remaining"), "win.column-left");
    g_menu_append (columns_submenu, _("Speed"), "win.column-speed");
    g_menu_append (columns_submenu, _("Upload Speed"), "win.column-upload-speed");
    g_menu_append (columns_submenu, _("Uploaded"), "win.column-uploaded");
    g_menu_append (columns_submenu, _("Ratio"), "win.column-ratio");
    g_menu_append (columns_submenu, _("Retry"), "win.column-retry");
    g_menu_append (columns_submenu, _("Category"), "win.column-category");
    g_menu_append (columns_submenu, _("URI"), "win.column-uri");
    g_menu_append (columns_submenu, _("Added On"), "win.column-added-on");
    g_menu_append (columns_submenu, _("Completed On"), "win.column-completed-on");
    g_menu_append_submenu (section, _("Download Columns"), G_MENU_MODEL (columns_submenu));
    g_object_unref (columns_submenu);
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);

    g_menu_append_submenu (menu_model, _("View"), G_MENU_MODEL (submenu));
    g_object_unref (submenu);

    // =========================================================================
    // --- Category Menu ---
    // =========================================================================
    submenu = g_menu_new ();
    g_menu_append (submenu, _("New Category"), "win.category-new");
    g_menu_append (submenu, _("Delete Category"), "win.category-delete");
    section = g_menu_new ();
    g_menu_append (section, _("Move Up"), "win.category-move-up");
    g_menu_append (section, _("Move Down"), "win.category-move-down");
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);
    section = g_menu_new ();
    g_menu_append (section, _("Properties"), "win.category-properties");
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);
    g_menu_append_submenu (menu_model, _("Category"), G_MENU_MODEL (submenu));
    g_object_unref (submenu);

    // =========================================================================
    // --- Download Menu ---
    // =========================================================================
    submenu = g_menu_new ();
    g_menu_append (submenu, _("New"), "win.download-new");
    g_menu_append (submenu, _("Delete Entry"), "win.download-delete");
    g_menu_append (submenu, _("Delete Entry and File"), "win.download-delete-file");
    
    section = g_menu_new ();
    section = g_menu_new ();
    g_menu_append (section, _("Open"), "win.download-open");
    g_menu_append (section, _("Open Containing Folder"), "win.download-open-folder");
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);

    section = g_menu_new ();
    g_menu_append (section, _("Force Start"), "win.download-force-start");
    g_menu_append (section, _("Runnable"), "win.download-start");
    g_menu_append (section, _("Pause"), "win.download-pause");
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);

    // Move To submenu (will be populated dynamically with categories) - in its own section
    section = g_menu_new ();
    GMenu *move_to_submenu = g_menu_new ();
    g_object_set_data (G_OBJECT (menu_model), "move-to-menu", move_to_submenu);
    g_menu_append_submenu (section, _("Move To"), G_MENU_MODEL (move_to_submenu));
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);

    section = g_menu_new ();
    g_menu_append (section, _("Move Up"), "win.download-move-up");
    g_menu_append (section, _("Move Down"), "win.download-move-down");
    g_menu_append (section, _("Move to Top"), "win.download-move-top");
    g_menu_append (section, _("Move to Bottom"), "win.download-move-bottom");
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);

    // Priority submenu - radio-style selection
    section = g_menu_new ();
    GMenu *priority_submenu = g_menu_new ();
    item = g_menu_item_new (_("High"), NULL);
    g_menu_item_set_action_and_target (item, "win.priority", "s", "high");
    g_menu_append_item (priority_submenu, item);
    g_object_unref (item);
    item = g_menu_item_new (_("Normal"), NULL);
    g_menu_item_set_action_and_target (item, "win.priority", "s", "normal");
    g_menu_append_item (priority_submenu, item);
    g_object_unref (item);
    item = g_menu_item_new (_("Low"), NULL);
    g_menu_item_set_action_and_target (item, "win.priority", "s", "low");
    g_menu_append_item (priority_submenu, item);
    g_object_unref (item);
    g_menu_append_submenu (section, _("Priority"), G_MENU_MODEL (priority_submenu));
    g_object_unref (priority_submenu);
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);

    section = g_menu_new ();
    g_menu_append (section, _("Properties"), "win.download-properties");
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);

    g_menu_append_submenu (menu_model, _("Download"), G_MENU_MODEL (submenu));
    g_object_unref (submenu);

    // =========================================================================
    // --- Help Menu ---
    // =========================================================================
    submenu = g_menu_new ();
    g_menu_append (submenu, _("Get Help Online"), "win.help-online");
    g_menu_append (submenu, _("Documentation"), "win.help-documentation");
    g_menu_append (submenu, _("Support Forum"), "win.help-support-forum");
    
    section = g_menu_new ();
    g_menu_append (section, _("Submit Feedback"), "win.help-feedback");
    g_menu_append (section, _("Report a Bug"), "win.help-report-bug");
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);
    
    section = g_menu_new ();
    g_menu_append (section, _("Check for Updates"), "win.help-check-updates");
    g_menu_append (section, _("About"), "win.help-about");
    g_menu_append_section (submenu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);
    
    g_menu_append_submenu (menu_model, _("Help"), G_MENU_MODEL (submenu));
    g_object_unref (submenu);

    // --- Create Popover Menu Bar ---
    menubar->self = gtk_popover_menu_bar_new_from_model (G_MENU_MODEL (menu_model));
    g_object_unref (menu_model);

	// Initialize all menu pointers to NULL (legacy)
	menubar->file.create_download = NULL;
	menubar->file.create_category = NULL;
	menubar->file.create_torrent = NULL;
	menubar->file.create_metalink = NULL;
	menubar->file.batch.clipboard = NULL;
	menubar->file.batch.sequence = NULL;
	menubar->file.batch.text_import = NULL;
	menubar->file.batch.html_import = NULL;
	menubar->file.batch.text_export = NULL;
	menubar->file.open_category = NULL;
	menubar->file.save_category = NULL;
	menubar->file.save = NULL;
	menubar->file.offline_mode = NULL;
	menubar->file.quit = NULL;
    menubar->edit.settings = NULL;
}

// Action Callbacks
static void on_action_quit (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_quit (app);
}

static void on_action_settings (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_open_settings (app);
}

static void on_action_new_download (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_create_download (app, NULL, NULL);
}

static void on_action_new_category (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_create_category (app);
}

static void on_action_delete_download (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_delete_download (app, FALSE);
}

static void on_action_delete_download_file (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_delete_download (app, TRUE);
}

static void on_action_delete_category (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_delete_category (app);
}

static void on_action_start_download (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_queue_download (app, TRUE);
}

static void on_action_pause_download (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_pause_download (app);
}

static void on_action_save (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_save (app);
}

// Move actions
static void on_action_move_up (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_move_download_up (app);
}

static void on_action_move_down (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_move_download_down (app);
}

static void on_action_move_top (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_move_download_top (app);
}

static void on_action_move_bottom (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_move_download_bottom (app);
}

// Properties
static void on_action_category_properties (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_edit_category (app);
}

static void on_action_download_properties (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_edit_download (app);
}

// Import/Export
static void on_action_import_text (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_import_text_file (app);
}

static void on_action_import_html (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_import_html_file (app);
}

static void on_action_export_text (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_export_text_file (app);
}

// Batch
static void on_action_batch_clipboard (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_clipboard_batch (app);
}

static void on_action_batch_sequence (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_sequence_batch (app);
}

// View toggles (toggle visibility of UI elements)
// View toggles (stateful - show checkmarks)
static void on_action_view_toolbar (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    GVariant *state = g_action_get_state (G_ACTION (action));
    gboolean new_state = !g_variant_get_boolean (state);
    g_simple_action_set_state (action, g_variant_new_boolean (new_state));
    g_variant_unref (state);
    gtk_widget_set_visible (app->toolbar.self, new_state);
    app->setting.window.toolbar = new_state;
}

static void on_action_view_statusbar (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    GVariant *state = g_action_get_state (G_ACTION (action));
    gboolean new_state = !g_variant_get_boolean (state);
    g_simple_action_set_state (action, g_variant_new_boolean (new_state));
    g_variant_unref (state);
    gtk_widget_set_visible (GTK_WIDGET (app->statusbar.self), new_state);
    app->setting.window.statusbar = new_state;
}

static void on_action_view_category (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    GVariant *state = g_action_get_state (G_ACTION (action));
    gboolean new_state = !g_variant_get_boolean (state);
    g_simple_action_set_state (action, g_variant_new_boolean (new_state));
    g_variant_unref (state);
    gtk_widget_set_visible (GTK_WIDGET(app->traveler.category.self), new_state);
    app->setting.window.category = new_state;
}

static void on_action_view_summary (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    GVariant *state = g_action_get_state (G_ACTION (action));
    gboolean new_state = !g_variant_get_boolean (state);
    g_simple_action_set_state (action, g_variant_new_boolean (new_state));
    g_variant_unref (state);
    gtk_widget_set_visible (app->summary.self, new_state);
    app->setting.window.summary = new_state;
}

// Offline mode toggle (stateful)
static void on_action_offline_mode (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    GVariant *state = g_action_get_state (G_ACTION (action));
    gboolean new_state = !g_variant_get_boolean (state);
    g_simple_action_set_state (action, g_variant_new_boolean (new_state));
    g_variant_unref (state);
    app->setting.offline_mode = new_state;
}

// Edit menu toggles (stateful - show checkmarks)
static void on_action_clipboard_monitor (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    GVariant *state = g_action_get_state (G_ACTION (action));
    gboolean new_state = !g_variant_get_boolean (state);
    g_simple_action_set_state (action, g_variant_new_boolean (new_state));
    g_variant_unref (state);
    app->setting.clipboard.monitor = new_state;
}

static void on_action_clipboard_quiet (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    GVariant *state = g_action_get_state (G_ACTION (action));
    gboolean new_state = !g_variant_get_boolean (state);
    g_simple_action_set_state (action, g_variant_new_boolean (new_state));
    g_variant_unref (state);
    app->setting.clipboard.quiet = new_state;
}

static void on_action_commandline_quiet (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    GVariant *state = g_action_get_state (G_ACTION (action));
    gboolean new_state = !g_variant_get_boolean (state);
    g_simple_action_set_state (action, g_variant_new_boolean (new_state));
    g_variant_unref (state);
    app->setting.commandline.quiet = new_state;
}

static void on_action_skip_existing (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    GVariant *state = g_action_get_state (G_ACTION (action));
    gboolean new_state = !g_variant_get_boolean (state);
    g_simple_action_set_state (action, g_variant_new_boolean (new_state));
    g_variant_unref (state);
    app->setting.ui.skip_existing = new_state;
}

static void on_action_apply_recent (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    GVariant *state = g_action_get_state (G_ACTION (action));
    gboolean new_state = !g_variant_get_boolean (state);
    g_simple_action_set_state (action, g_variant_new_boolean (new_state));
    g_variant_unref (state);
    app->setting.ui.apply_recent = new_state;
}

// Download Open actions
static void on_action_download_open (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    UgetNode *node = app->traveler.download.cursor.node;
    if (node == NULL)
        return;
    node = node->base;
    
    UgetCommon *common = ug_info_get (node->info, UgetCommonInfo);
    if (common == NULL || common->file == NULL || common->folder == NULL)
        return;
    
    char *path = g_build_filename (common->folder, common->file, NULL);
#if defined _WIN32 || defined _WIN64
    ShellExecuteA (NULL, "open", path, NULL, NULL, SW_SHOWNORMAL);
#else
    char *command = g_strdup_printf ("xdg-open \"%s\"", path);
    g_spawn_command_line_async (command, NULL);
    g_free (command);
#endif
    g_free (path);
}

static void on_action_download_open_folder (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    UgetNode *node = app->traveler.download.cursor.node;
    if (node == NULL)
        return;
    node = node->base;
    
    UgetCommon *common = ug_info_get (node->info, UgetCommonInfo);
    if (common == NULL || common->folder == NULL)
        return;
    
#if defined _WIN32 || defined _WIN64
    ShellExecuteA (NULL, "explore", common->folder, NULL, NULL, SW_SHOWNORMAL);
#else
    char *command = g_strdup_printf ("xdg-open \"%s\"", common->folder);
    g_spawn_command_line_async (command, NULL);
    g_free (command);
#endif
}

// Help menu - open URLs in browser
static void open_url_in_browser (const char* url) {
#if defined _WIN32 || defined _WIN64
    ShellExecuteA (NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
#else
    char* command = g_strdup_printf ("xdg-open %s", url);
    g_spawn_command_line_async (command, NULL);
    g_free (command);
#endif
}

static void on_action_help_online (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    open_url_in_browser ("https://ugetdm.com/help/");
}

static void on_action_help_documentation (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    open_url_in_browser ("https://ugetdm.com/documentation/");
}

static void on_action_help_support_forum (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    open_url_in_browser ("https://sourceforge.net/p/urlget/discussion/");
}

static void on_action_help_feedback (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    open_url_in_browser ("https://ugetdm.com/feedback/");
}

static void on_action_help_report_bug (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    open_url_in_browser ("https://github.com/ugetdm/uget/issues");
}

static void on_action_help_check_updates (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    open_url_in_browser ("https://ugetdm.com/downloads/");
}

static void on_action_help_about (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    UgtkAboutDialog* adialog = ugtk_about_dialog_new (app->window.self);
    ugtk_about_dialog_run (adialog);
}

// Open/Save Category actions
static void on_action_open_category (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_load_category (app);
}

static void on_action_save_category (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    ugtk_app_save_category (app);
}

// Completion actions - radio-style with string state
static void on_action_completion_action (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    const gchar *value = g_variant_get_string (parameter, NULL);
    int action_value = 0;
    
    if (g_str_equal (value, "disable"))
        action_value = 0;
    else if (g_str_equal (value, "hibernate"))
        action_value = 1;
    else if (g_str_equal (value, "suspend"))
        action_value = 2;
    else if (g_str_equal (value, "shutdown"))
        action_value = 3;
    else if (g_str_equal (value, "reboot"))
        action_value = 4;
    else if (g_str_equal (value, "custom"))
        action_value = 5;
    
    app->setting.completion.action = action_value;
    g_simple_action_set_state (action, parameter);
}

static void on_action_completion_remember (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    GVariant *state = g_action_get_state (G_ACTION (action));
    gboolean new_state = !g_variant_get_boolean (state);
    g_simple_action_set_state (action, g_variant_new_boolean (new_state));
    g_variant_unref (state);
    app->setting.completion.remember = new_state;
}

static void on_action_completion_help (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    open_url_in_browser ("https://ugetdm.com/help/completion/");
}

// Category move actions
static void on_action_category_move_up (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    UgetNode *cnode = app->traveler.category.cursor.node;
    if (cnode == NULL)
        return;
    cnode = cnode->base;
    UgetNode *position = cnode->prev;
    if (position) {
        gint new_pos = app->traveler.category.cursor.pos - 1;
        uget_app_move_category ((UgetApp*) app, cnode, position);
        gtk_widget_queue_draw ((GtkWidget*) app->traveler.category.view);
        // Re-select the moved category at its new position
        ugtk_traveler_select_category (&app->traveler, new_pos, -1);
    }
}

static void on_action_category_move_down (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    UgetNode *cnode = app->traveler.category.cursor.node;
    if (cnode == NULL)
        return;
    cnode = cnode->base;
    UgetNode *position = cnode->next;
    if (position && position->next)
        position = position->next;
    else
        position = NULL;  // Move to end
    if (position || cnode->next) {  // Can only move if there's a next
        gint new_pos = app->traveler.category.cursor.pos + 1;
        uget_app_move_category ((UgetApp*) app, cnode, position);
        gtk_widget_queue_draw ((GtkWidget*) app->traveler.category.view);
        // Re-select the moved category at its new position
        ugtk_traveler_select_category (&app->traveler, new_pos, -1);
    }
}

// Move To Category action - moves selected downloads to a category by index
static void on_action_move_to_category (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    gint32 category_index = g_variant_get_int32 (parameter);
    
    // Get the target category node by index
    UgetNode *cnode = app->real.children;
    for (int i = 0; i < category_index && cnode; i++)
        cnode = cnode->next;
    
    if (cnode == NULL)
        return;
    
    // Move selected downloads to this category
    ugtk_app_move_download_to (app, cnode);
}

// Priority actions - radio-style with string state
static void on_action_priority (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    const gchar *value = g_variant_get_string (parameter, NULL);
    UgetPriority priority = UGET_PRIORITY_NORMAL;
    
    if (g_str_equal (value, "high"))
        priority = UGET_PRIORITY_HIGH;
    else if (g_str_equal (value, "normal"))
        priority = UGET_PRIORITY_NORMAL;
    else if (g_str_equal (value, "low"))
        priority = UGET_PRIORITY_LOW;
    
    UgetNode *node = app->traveler.download.cursor.node;
    if (node == NULL)
        return;
    node = node->base;
    UgetRelation *relation = ug_info_get (node->info, UgetRelationInfo);
    if (relation)
        relation->priority = priority;
    g_simple_action_set_state (action, parameter);
}

// Summary copy actions
static void on_action_summary_copy (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    gchar *text = ugtk_summary_get_text_selected (&app->summary);
    if (text) {
        ugtk_clipboard_set_text (&app->clipboard, text);
        g_free (text);
    }
}

static void on_action_summary_copy_all (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    gchar *text = ugtk_summary_get_text_all (&app->summary);
    if (text) {
        ugtk_clipboard_set_text (&app->clipboard, text);
        g_free (text);
    }
}

// View Summary items - update summary->visible and rebuild display
static void on_action_summary_item (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    const gchar *name = g_action_get_name (G_ACTION (action));
    GVariant *state = g_action_get_state (G_ACTION (action));
    gboolean new_state = !g_variant_get_boolean (state);
    g_simple_action_set_state (action, g_variant_new_boolean (new_state));
    g_variant_unref (state);
    
    // Update both app setting and summary->visible struct
    if (g_str_equal (name, "summary-name")) {
        app->setting.summary.name = new_state;
        app->summary.visible.name = new_state;
    } else if (g_str_equal (name, "summary-folder")) {
        app->setting.summary.folder = new_state;
        app->summary.visible.folder = new_state;
    } else if (g_str_equal (name, "summary-category")) {
        app->setting.summary.category = new_state;
        app->summary.visible.category = new_state;
    } else if (g_str_equal (name, "summary-uri")) {
        app->setting.summary.uri = new_state;
        app->summary.visible.uri = new_state;
    } else if (g_str_equal (name, "summary-message")) {
        app->setting.summary.message = new_state;
        app->summary.visible.message = new_state;
    }
    
    // Rebuild the summary display
    ugtk_summary_show (&app->summary, app->traveler.download.cursor.node ? app->traveler.download.cursor.node->base : NULL);
}

// View Column toggles
static void on_action_column_toggle (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    UgtkApp *app = (UgtkApp*) user_data;
    const gchar *name = g_action_get_name (G_ACTION (action));
    GVariant *state = g_action_get_state (G_ACTION (action));
    gboolean new_state = !g_variant_get_boolean (state);
    g_simple_action_set_state (action, g_variant_new_boolean (new_state));
    g_variant_unref (state);
    
    GtkTreeView *view = app->traveler.download.view;
    GtkTreeViewColumn *column = NULL;
    int column_index = -1;
    
    // Toggle column visibility in setting and apply to UI
    if (g_str_equal (name, "column-complete")) {
        app->setting.download_column.complete = new_state;
        column_index = UGTK_NODE_COLUMN_COMPLETE;
    } else if (g_str_equal (name, "column-total")) {
        app->setting.download_column.total = new_state;
        column_index = UGTK_NODE_COLUMN_TOTAL;
    } else if (g_str_equal (name, "column-percent")) {
        app->setting.download_column.percent = new_state;
        column_index = UGTK_NODE_COLUMN_PERCENT;
    } else if (g_str_equal (name, "column-elapsed")) {
        app->setting.download_column.elapsed = new_state;
        column_index = UGTK_NODE_COLUMN_ELAPSED;
    } else if (g_str_equal (name, "column-left")) {
        app->setting.download_column.left = new_state;
        column_index = UGTK_NODE_COLUMN_LEFT;
    } else if (g_str_equal (name, "column-speed")) {
        app->setting.download_column.speed = new_state;
        column_index = UGTK_NODE_COLUMN_SPEED;
    } else if (g_str_equal (name, "column-upload-speed")) {
        app->setting.download_column.upload_speed = new_state;
        column_index = UGTK_NODE_COLUMN_UPLOAD_SPEED;
    } else if (g_str_equal (name, "column-uploaded")) {
        app->setting.download_column.uploaded = new_state;
        column_index = UGTK_NODE_COLUMN_UPLOADED;
    } else if (g_str_equal (name, "column-ratio")) {
        app->setting.download_column.ratio = new_state;
        column_index = UGTK_NODE_COLUMN_RATIO;
    } else if (g_str_equal (name, "column-retry")) {
        app->setting.download_column.retry = new_state;
        column_index = UGTK_NODE_COLUMN_RETRY;
    } else if (g_str_equal (name, "column-category")) {
        app->setting.download_column.category = new_state;
        column_index = UGTK_NODE_COLUMN_CATEGORY;
    } else if (g_str_equal (name, "column-uri")) {
        app->setting.download_column.uri = new_state;
        column_index = UGTK_NODE_COLUMN_URI;
    } else if (g_str_equal (name, "column-added-on")) {
        app->setting.download_column.added_on = new_state;
        column_index = UGTK_NODE_COLUMN_ADDED_ON;
    } else if (g_str_equal (name, "column-completed-on")) {
        app->setting.download_column.completed_on = new_state;
        column_index = UGTK_NODE_COLUMN_COMPLETED_ON;
    }
    
    // Apply visibility change to column
    if (column_index >= 0) {
        column = gtk_tree_view_get_column (view, column_index);
        if (column)
            gtk_tree_view_column_set_visible (column, new_state);
    }
}

void ugtk_menubar_register_actions (UgtkApp *app)
{
    // Non-stateful actions
    const GActionEntry app_entries[] = {
        // File menu
        { "new-download", on_action_new_download, NULL, NULL, NULL, {0} },
        { "new-category", on_action_new_category, NULL, NULL, NULL, {0} },
        { "batch-clipboard", on_action_batch_clipboard, NULL, NULL, NULL, {0} },
        { "batch-sequence", on_action_batch_sequence, NULL, NULL, NULL, {0} },
        { "import-text", on_action_import_text, NULL, NULL, NULL, {0} },
        { "import-html", on_action_import_html, NULL, NULL, NULL, {0} },
        { "export-text", on_action_export_text, NULL, NULL, NULL, {0} },
        { "open-category", on_action_open_category, NULL, NULL, NULL, {0} },
        { "save-category", on_action_save_category, NULL, NULL, NULL, {0} },
        { "save", on_action_save, NULL, NULL, NULL, {0} },
        { "quit", on_action_quit, NULL, NULL, NULL, {0} },
        { "settings", on_action_settings, NULL, NULL, NULL, {0} },

        // Edit - Completion (only non-radio items)
        { "completion-help", on_action_completion_help, NULL, NULL, NULL, {0} },

        // Category menu
        { "category-new", on_action_new_category, NULL, NULL, NULL, {0} },
        { "category-delete", on_action_delete_category, NULL, NULL, NULL, {0} },
        { "category-move-up", on_action_category_move_up, NULL, NULL, NULL, {0} },
        { "category-move-down", on_action_category_move_down, NULL, NULL, NULL, {0} },
        { "category-properties", on_action_category_properties, NULL, NULL, NULL, {0} },

        // Download menu
        { "download-new", on_action_new_download, NULL, NULL, NULL, {0} },
        { "download-delete", on_action_delete_download, NULL, NULL, NULL, {0} },
        { "download-delete-file", on_action_delete_download_file, NULL, NULL, NULL, {0} },
        { "download-open", on_action_download_open, NULL, NULL, NULL, {0} },
        { "download-open-folder", on_action_download_open_folder, NULL, NULL, NULL, {0} },
        { "download-force-start", on_action_start_download, NULL, NULL, NULL, {0} },
        { "download-start", on_action_start_download, NULL, NULL, NULL, {0} },
        { "download-pause", on_action_pause_download, NULL, NULL, NULL, {0} },
        { "download-move-up", on_action_move_up, NULL, NULL, NULL, {0} },
        { "download-move-down", on_action_move_down, NULL, NULL, NULL, {0} },
        { "download-move-top", on_action_move_top, NULL, NULL, NULL, {0} },
        { "download-move-bottom", on_action_move_bottom, NULL, NULL, NULL, {0} },
        { "download-properties", on_action_download_properties, NULL, NULL, NULL, {0} },

        // Help menu
        { "help-online", on_action_help_online, NULL, NULL, NULL, {0} },
        { "help-documentation", on_action_help_documentation, NULL, NULL, NULL, {0} },
        { "help-support-forum", on_action_help_support_forum, NULL, NULL, NULL, {0} },
        { "help-feedback", on_action_help_feedback, NULL, NULL, NULL, {0} },
        { "help-report-bug", on_action_help_report_bug, NULL, NULL, NULL, {0} },
        { "help-check-updates", on_action_help_check_updates, NULL, NULL, NULL, {0} },
        { "help-about", on_action_help_about, NULL, NULL, NULL, {0} },

        // Summary context menu
        { "summary-copy", on_action_summary_copy, NULL, NULL, NULL, {0} },
        { "summary-copy-all", on_action_summary_copy_all, NULL, NULL, NULL, {0} },
    };

    GSimpleActionGroup *action_group = g_simple_action_group_new ();
    g_action_map_add_action_entries (G_ACTION_MAP (action_group),
                                     app_entries,
                                     G_N_ELEMENTS (app_entries),
                                     app);

    // Stateful toggle actions (with checkmarks)
    GSimpleAction *action;

    // Offline mode
    action = g_simple_action_new_stateful ("offline-mode", NULL, 
                                           g_variant_new_boolean (app->setting.offline_mode));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_offline_mode), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    // Clipboard monitor
    action = g_simple_action_new_stateful ("clipboard-monitor", NULL,
                                           g_variant_new_boolean (app->setting.clipboard.monitor));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_clipboard_monitor), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    // Clipboard quiet
    action = g_simple_action_new_stateful ("clipboard-quiet", NULL,
                                           g_variant_new_boolean (app->setting.clipboard.quiet));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_clipboard_quiet), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    // Commandline quiet
    action = g_simple_action_new_stateful ("commandline-quiet", NULL,
                                           g_variant_new_boolean (app->setting.commandline.quiet));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_commandline_quiet), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    // Skip existing
    action = g_simple_action_new_stateful ("skip-existing", NULL,
                                           g_variant_new_boolean (app->setting.ui.skip_existing));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_skip_existing), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    // Apply recent
    action = g_simple_action_new_stateful ("apply-recent", NULL,
                                           g_variant_new_boolean (app->setting.ui.apply_recent));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_apply_recent), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    // Move to category - action with int32 parameter for category index
    action = g_simple_action_new ("move-to-category", G_VARIANT_TYPE_INT32);
    g_signal_connect (action, "activate", G_CALLBACK (on_action_move_to_category), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    // Completion action - radio-style with string state
    static const gchar *completion_values[] = {"disable", "hibernate", "suspend", "shutdown", "reboot", "custom"};
    const gchar *completion_state = completion_values[app->setting.completion.action < 6 ? app->setting.completion.action : 0];
    action = g_simple_action_new_stateful ("completion-action", G_VARIANT_TYPE_STRING,
                                           g_variant_new_string (completion_state));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_completion_action), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    // Priority - radio-style with string state (default to "normal")
    action = g_simple_action_new_stateful ("priority", G_VARIANT_TYPE_STRING,
                                           g_variant_new_string ("normal"));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_priority), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    // View menu - stateful toggles
    action = g_simple_action_new_stateful ("view-toolbar", NULL,
                                           g_variant_new_boolean (app->setting.window.toolbar));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_view_toolbar), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("view-statusbar", NULL,
                                           g_variant_new_boolean (app->setting.window.statusbar));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_view_statusbar), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("view-category", NULL,
                                           g_variant_new_boolean (app->setting.window.category));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_view_category), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("view-summary", NULL,
                                           g_variant_new_boolean (app->setting.window.summary));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_view_summary), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    // Completion - Remember toggle
    action = g_simple_action_new_stateful ("completion-remember", NULL,
                                           g_variant_new_boolean (app->setting.completion.remember));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_completion_remember), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    // Summary items - stateful toggles
    action = g_simple_action_new_stateful ("summary-name", NULL,
                                           g_variant_new_boolean (app->setting.summary.name));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_summary_item), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("summary-folder", NULL,
                                           g_variant_new_boolean (app->setting.summary.folder));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_summary_item), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("summary-category", NULL,
                                           g_variant_new_boolean (app->setting.summary.category));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_summary_item), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("summary-uri", NULL,
                                           g_variant_new_boolean (app->setting.summary.uri));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_summary_item), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("summary-message", NULL,
                                           g_variant_new_boolean (app->setting.summary.message));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_summary_item), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    // Download columns - stateful toggles
    action = g_simple_action_new_stateful ("column-complete", NULL,
                                           g_variant_new_boolean (app->setting.download_column.complete));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_column_toggle), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("column-total", NULL,
                                           g_variant_new_boolean (app->setting.download_column.total));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_column_toggle), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("column-percent", NULL,
                                           g_variant_new_boolean (app->setting.download_column.percent));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_column_toggle), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("column-elapsed", NULL,
                                           g_variant_new_boolean (app->setting.download_column.elapsed));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_column_toggle), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("column-left", NULL,
                                           g_variant_new_boolean (app->setting.download_column.left));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_column_toggle), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("column-speed", NULL,
                                           g_variant_new_boolean (app->setting.download_column.speed));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_column_toggle), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("column-upload-speed", NULL,
                                           g_variant_new_boolean (app->setting.download_column.upload_speed));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_column_toggle), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("column-uploaded", NULL,
                                           g_variant_new_boolean (app->setting.download_column.uploaded));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_column_toggle), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("column-ratio", NULL,
                                           g_variant_new_boolean (app->setting.download_column.ratio));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_column_toggle), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("column-retry", NULL,
                                           g_variant_new_boolean (app->setting.download_column.retry));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_column_toggle), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("column-category", NULL,
                                           g_variant_new_boolean (app->setting.download_column.category));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_column_toggle), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("column-uri", NULL,
                                           g_variant_new_boolean (app->setting.download_column.uri));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_column_toggle), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("column-added-on", NULL,
                                           g_variant_new_boolean (app->setting.download_column.added_on));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_column_toggle), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);

    action = g_simple_action_new_stateful ("column-completed-on", NULL,
                                           g_variant_new_boolean (app->setting.download_column.completed_on));
    g_signal_connect (action, "activate", G_CALLBACK (on_action_column_toggle), app);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    g_object_unref (action);
    
    // Insert action group 'win' into the main window
    gtk_widget_insert_action_group (GTK_WIDGET (app->window.self),
                                    "win",
                                    G_ACTION_GROUP (action_group));
    // Store reference for later use (e.g., priority updates on selection change)
    app->action_group = action_group;  // Don't unref - keep reference
}

// =============================================================================
// --- Popover Menu Hover Behavior (GTK3-like) ---
// =============================================================================

// CSS class for hover highlight
#define HOVER_CLASS "uget-hover"

// One-time CSS provider installation flag
static gboolean hover_css_installed = FALSE;

// Install CSS provider for hover class (called once)
static void ensure_hover_css_installed (void)
{
    if (hover_css_installed)
        return;
    hover_css_installed = TRUE;
    
    GtkCssProvider *css = gtk_css_provider_new ();

#if defined _WIN32 || defined _WIN64
    // Windows: Neutralize GNOME-ish styling and enforce Windows Blue
    gtk_css_provider_load_from_string (css,
        // Reset rounded corners and shadows
        "popover.menu, "
        "popover.menu > contents, "
        "popover.menu > contents > * { "
        "  border-radius: 0px; box-shadow: none; background-image: none; "
        "} "
        "popover.menu > contents { "
        "  background-color: #ffffff; border: 1px solid #888; padding: 2px; "
        "} "
        "popover.menu > arrow { background: transparent; border: none; } "

        // Basic clean button style (normal state)
        "modelbutton, "
        "button.model { "
        "  min-height: 22px; "
        "  border-radius: 0px; "
        "  color: #000000; "
        "  background-color: #ffffff; "
        "  background-image: none; "
        "  transition: none; "
        "} "
        
        // HOVER STATE - Use native :hover pseudo-class (maps to GTK_STATE_FLAG_PRELIGHT)
        "modelbutton:hover, "
        "button.model:hover { "
        "  background-color: #3584e4 !important; "
        "  color: #ffffff !important; "
        "  background-image: none !important; "
        "} "
        
        // Force ALL children (labels, images) to be white on hover
        "modelbutton:hover *, "
        "button.model:hover * { "
        "  color: #ffffff !important; "
        "} "
    );
#else
    gtk_css_provider_load_from_string (css,
        "popover.menu .uget-hover, "
        "popover.menu .uget-hover * { "
        "  background-image: none; "
        "  background-color: alpha(@theme_fg_color, 0.1); "
        "}"
    );
#endif

    // Use APPLICATION priority (highest built-in) to override theme
    gtk_style_context_add_provider_for_display (
        gdk_display_get_default (),
        GTK_STYLE_PROVIDER (css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 100
    );
    g_object_unref (css);
}

// Walk widget tree recursively
static void popover_walk_widgets (GtkWidget *w, GFunc fn, gpointer user_data)
{
    fn (w, user_data);
    for (GtkWidget *c = gtk_widget_get_first_child (w); c != NULL; c = gtk_widget_get_next_sibling (c)) {
        popover_walk_widgets (c, fn, user_data);
    }
}

// Key for storing the hover CSS provider on the widget
#define HOVER_CSS_KEY "uget-hover-css-provider"

#if defined _WIN32 || defined _WIN64
// Shared CSS providers (created once)
static GtkCssProvider *hover_bg_css = NULL;   // For button background
static GtkCssProvider *hover_text_css = NULL; // For label text color

static void ensure_hover_providers (void)
{
    if (!hover_bg_css) {
        hover_bg_css = gtk_css_provider_new ();
        gtk_css_provider_load_from_string (hover_bg_css,
            "* { "
            "  background-color: #3584e4; "
            "  background-image: none; "
            "}"
        );
    }
    if (!hover_text_css) {
        hover_text_css = gtk_css_provider_new ();
        // Use wildcard and !important to force white text
        gtk_css_provider_load_from_string (hover_text_css,
            "* { "
            "  color: #ffffff !important; "
            "  -gtk-icon-palette: success #ffffff, warning #ffffff, error #ffffff; "
            "}"
        );
    }
}

// Key for storing original label text
#define LABEL_ORIG_TEXT_KEY "uget-label-orig-text"

// Find first GtkLabel child (shallow search, max 2 levels)
static GtkLabel* find_label_child (GtkWidget *parent)
{
    for (GtkWidget *c = gtk_widget_get_first_child (parent); c != NULL; c = gtk_widget_get_next_sibling (c)) {
        if (GTK_IS_LABEL (c))
            return GTK_LABEL (c);
        // Check grandchildren too
        for (GtkWidget *gc = gtk_widget_get_first_child (c); gc != NULL; gc = gtk_widget_get_next_sibling (gc)) {
            if (GTK_IS_LABEL (gc))
                return GTK_LABEL (gc);
        }
    }
    return NULL;
}

// Apply hover styling to a single button row
// Uses per-widget CSS for background + Pango markup for text color
static void apply_hover_to_row (GtkWidget *row, gboolean add)
{
    ensure_hover_providers ();
    
    // Apply background CSS to the row itself via per-widget provider
    GtkStyleContext *ctx = gtk_widget_get_style_context (row);
    if (add) {
        gtk_style_context_add_provider (ctx, GTK_STYLE_PROVIDER (hover_bg_css), G_MAXUINT);
    } else {
        gtk_style_context_remove_provider (ctx, GTK_STYLE_PROVIDER (hover_bg_css));
    }
    
    // Find the label and apply Pango markup for text color
    // (CSS color doesn't work on labels in GTK4!)
    GtkLabel *label = find_label_child (row);
    if (label) {
        GtkWidget *lw = GTK_WIDGET (label);
        if (add) {
            const gchar *orig = gtk_label_get_text (label);
            if (orig && *orig) {
                g_object_set_data_full (G_OBJECT (lw), LABEL_ORIG_TEXT_KEY, g_strdup (orig), g_free);
                gchar *markup = g_markup_printf_escaped ("<span foreground=\"#ffffff\">%s</span>", orig);
                gtk_label_set_markup (label, markup);
                g_free (markup);
            }
        } else {
            const gchar *orig = g_object_get_data (G_OBJECT (lw), LABEL_ORIG_TEXT_KEY);
            if (orig) {
                gtk_label_set_text (label, orig);
                g_object_set_data (G_OBJECT (lw), LABEL_ORIG_TEXT_KEY, NULL);
            }
        }
    }
}
#endif

static void on_popover_row_enter (GtkEventControllerMotion *motion, double x, double y, gpointer user_data)
{
    GtkWidget *row = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (motion));
    if (!row) return;

#if defined _WIN32 || defined _WIN64
    apply_hover_to_row (row, TRUE);
#else
    gtk_widget_add_css_class (row, HOVER_CLASS);
#endif
    gtk_widget_queue_draw (row);
}

static void on_popover_row_leave (GtkEventControllerMotion *motion, gpointer user_data)
{
    GtkWidget *row = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (motion));
    if (!row) return;

#if defined _WIN32 || defined _WIN64
    apply_hover_to_row (row, FALSE);
#else
    gtk_widget_remove_css_class (row, HOVER_CLASS);
#endif
    gtk_widget_queue_draw (row);
}
// Check if widget has 'model' CSS class (menu row)
static gboolean widget_has_model_class (GtkWidget *w)
{
    return gtk_widget_has_css_class (w, "model");
}

// Attach motion controller to menu-row widgets
static void popover_maybe_install_hover (GtkWidget *w, gpointer user_data)
{
    const char *type_name = G_OBJECT_TYPE_NAME (w);
    gboolean is_model_button = (g_strcmp0 (type_name, "GtkModelButton") == 0);
    gboolean has_model_class = widget_has_model_class (w);
    
    if (!is_model_button && !has_model_class && !GTK_IS_BUTTON (w))
        return;
    
    // Avoid installing twice
    if (g_object_get_data (G_OBJECT (w), "hover-controller-installed"))
        return;
        
    g_object_set_data (G_OBJECT (w), "hover-controller-installed", GINT_TO_POINTER (1));
    
    GtkEventController *motion = gtk_event_controller_motion_new ();
    gtk_event_controller_set_propagation_phase (motion, GTK_PHASE_CAPTURE);
    g_signal_connect (motion, "enter", G_CALLBACK (on_popover_row_enter), user_data);
    g_signal_connect (motion, "leave", G_CALLBACK (on_popover_row_leave), user_data);
    gtk_widget_add_controller (w, motion);
    
    // Prevent initial gray selection by clearing flags on install
    gtk_widget_unset_state_flags (w, GTK_STATE_FLAG_SELECTED | GTK_STATE_FLAG_PRELIGHT);
}

// Install hover behavior on all rows in popover
static void install_popover_hover_behavior (GtkPopoverMenu *pmenu)
{
    GtkWidget *root = GTK_WIDGET (pmenu);
    popover_walk_widgets (root, (GFunc) popover_maybe_install_hover, pmenu);
}

// Idle callback to install hover behavior 
static gboolean install_hover_idle (gpointer user_data)
{
    GtkPopoverMenu *pmenu = GTK_POPOVER_MENU (user_data);
    if (gtk_widget_get_visible (GTK_WIDGET (pmenu)))
        install_popover_hover_behavior (pmenu);
    return G_SOURCE_REMOVE;
}

// On visible notify: defer install to idle to ensure tree is built
static void on_popover_visible_notify (GObject *obj, GParamSpec *pspec, gpointer user_data)
{
    GtkWidget *popover = GTK_WIDGET (obj);
    if (gtk_widget_get_visible (popover))
        g_idle_add (install_hover_idle, popover);
}

// Hook any GtkPopover found inside the menubar to install hover behavior
static void hook_menubar_popover (GtkWidget *w, gpointer user_data)
{
    // Check if this is a GtkPopoverMenu (or any popover subclass)
    if (GTK_IS_POPOVER (w)) {
        // Avoid double-hooking
        if (!g_object_get_data (G_OBJECT (w), "menubar-hover-hooked")) {
            g_object_set_data (G_OBJECT (w), "menubar-hover-hooked", GINT_TO_POINTER (1));
            g_signal_connect (w, "notify::visible", G_CALLBACK (on_popover_visible_notify), NULL);
        }
    }
}



// =============================================================================
// Context Menu Functions for GTK4
// =============================================================================

// Create Category context menu model
static GMenu* ugtk_create_category_context_menu (void)
{
    GMenu *menu = g_menu_new ();
    g_menu_append (menu, _("New Category"), "win.category-new");
    g_menu_append (menu, _("Delete Category"), "win.category-delete");
    
    GMenu *section = g_menu_new ();
    g_menu_append (section, _("Move Up"), "win.category-move-up");
    g_menu_append (section, _("Move Down"), "win.category-move-down");
    g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);
    
    section = g_menu_new ();
    g_menu_append (section, _("Properties"), "win.category-properties");
    g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);
    
    return menu;
}

// Create Download context menu model
static GMenu* ugtk_create_download_context_menu (void)
{
    GMenu *menu = g_menu_new ();
    g_menu_append (menu, _("New"), "win.download-new");
    g_menu_append (menu, _("Delete Entry"), "win.download-delete");
    g_menu_append (menu, _("Delete Entry and File"), "win.download-delete-file");
    
    GMenu *section = g_menu_new ();
    g_menu_append (section, _("Open"), "win.download-open");
    g_menu_append (section, _("Open Containing Folder"), "win.download-open-folder");
    g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);
    
    section = g_menu_new ();
    g_menu_append (section, _("Force Start"), "win.download-force-start");
    g_menu_append (section, _("Runnable"), "win.download-start");
    g_menu_append (section, _("Pause"), "win.download-pause");
    g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);

    // Move To submenu (dynamically populated with categories) - in its own section
    section = g_menu_new ();
    GMenu *move_to_submenu = g_menu_new ();
    g_object_set_data (G_OBJECT (menu), "move-to-menu", move_to_submenu);
    g_menu_append_submenu (section, _("Move To"), G_MENU_MODEL (move_to_submenu));
    g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);

    section = g_menu_new ();
    g_menu_append (section, _("Move Up"), "win.download-move-up");
    g_menu_append (section, _("Move Down"), "win.download-move-down");
    g_menu_append (section, _("Move to Top"), "win.download-move-top");
    g_menu_append (section, _("Move to Bottom"), "win.download-move-bottom");
    g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);

    // Priority submenu - radio-style selection
    section = g_menu_new ();
    GMenu *priority_submenu = g_menu_new ();
    GMenuItem *item = g_menu_item_new (_("High"), NULL);
    g_menu_item_set_action_and_target (item, "win.priority", "s", "high");
    g_menu_append_item (priority_submenu, item);
    g_object_unref (item);
    item = g_menu_item_new (_("Normal"), NULL);
    g_menu_item_set_action_and_target (item, "win.priority", "s", "normal");
    g_menu_append_item (priority_submenu, item);
    g_object_unref (item);
    item = g_menu_item_new (_("Low"), NULL);
    g_menu_item_set_action_and_target (item, "win.priority", "s", "low");
    g_menu_append_item (priority_submenu, item);
    g_object_unref (item);
    g_menu_append_submenu (section, _("Priority"), G_MENU_MODEL (priority_submenu));
    g_object_unref (priority_submenu);
    g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);
   
    section = g_menu_new ();
    g_menu_append (section, _("Properties"), "win.download-properties");
    g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
    g_object_unref (section);
    
    return menu;
}

// Create Summary context menu model
static GMenu* ugtk_create_summary_context_menu (void)
{
    GMenu *menu = g_menu_new ();
    g_menu_append (menu, _("Copy"), "win.summary-copy");
    g_menu_append (menu, _("Copy All"), "win.summary-copy-all");
    return menu;
}

// Populate a Move To menu with category names
// menu: The GMenu to populate (clear and refill)
// app: The application instance
// current_category_pos: The currently selected category position (to mark current disabled)
static void ugtk_populate_move_to_menu (GMenu *menu, UgtkApp *app, gint current_category_pos)
{
    if (menu == NULL)
        return;
    
    // Clear existing items
    g_menu_remove_all (menu);
    
    // Add category items
    UgetNode *cnode = app->real.children;
    gint index = 0;
    while (cnode) {
        UgetCommon *common = ug_info_get (cnode->info, UgetCommonInfo);
        const gchar *name = (common && common->name) ? common->name : _("Unnamed");
        
        // Create menu item with action target (category index)
        GMenuItem *item = g_menu_item_new (name, NULL);
        g_menu_item_set_action_and_target (item, "win.move-to-category", "i", index);
        g_menu_append_item (menu, item);
        g_object_unref (item);
        
        cnode = cnode->next;
        index++;
    }
}

// Gesture callback for right-click on Category TreeView
static void on_category_right_click (GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data)
{
    UgtkApp *app = (UgtkApp*) user_data;
    GdkRectangle rect = {(int)x, (int)y, 1, 1};
    
    // Select the row under the cursor so cursor state is updated before we read it
    GtkTreePath *path = NULL;
    if (gtk_tree_view_get_path_at_pos (app->traveler.category.view, (int)x, (int)y, &path, NULL, NULL, NULL)) {
        GtkTreeSelection *selection = gtk_tree_view_get_selection (app->traveler.category.view);
        gtk_tree_selection_select_path (selection, path);
        gtk_tree_view_set_cursor (app->traveler.category.view, path, NULL, FALSE);
        gtk_tree_path_free (path);
    }
    
    // Enable/disable actions based on selection
    // pos == 0 means "All Category" is selected - only New Category should be enabled
    gboolean is_all_category = (app->traveler.category.cursor.pos == 0);
    gboolean is_first_category = (app->traveler.category.cursor.pos <= 1);
    gboolean is_last_category = (app->traveler.category.cursor.node && 
                                  app->traveler.category.cursor.node->next == NULL);
    
    GAction *action;
    action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "category-delete");
    if (action)
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), !is_all_category);
    
    action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "category-move-up");
    if (action)
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), !is_all_category && !is_first_category);
    
    action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "category-move-down");
    if (action)
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), !is_all_category && !is_last_category);
    
    action = g_action_map_lookup_action (G_ACTION_MAP (app->action_group), "category-properties");
    if (action)
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), !is_all_category);
    
    gtk_popover_set_pointing_to (GTK_POPOVER (app->category_context_menu), &rect);
    gtk_popover_popup (GTK_POPOVER (app->category_context_menu));
}

// Gesture callback for right-click on Download TreeView
static void on_download_right_click (GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data)
{
    UgtkApp *app = (UgtkApp*) user_data;
    GdkRectangle rect = {(int)x, (int)y, 1, 1};
    
    // Populate Move To submenu with current categories
    GMenuModel *menu_model = gtk_popover_menu_get_menu_model (GTK_POPOVER_MENU (app->download_context_menu));
    GMenu *move_to_menu = g_object_get_data (G_OBJECT (menu_model), "move-to-menu");
    ugtk_populate_move_to_menu (move_to_menu, app, app->traveler.category.cursor.pos);
    
    gtk_popover_set_pointing_to (GTK_POPOVER (app->download_context_menu), &rect);
    gtk_popover_popup (GTK_POPOVER (app->download_context_menu));
}

// Gesture callback for right-click on Summary TreeView
static void on_summary_right_click (GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data)
{
    UgtkApp *app = (UgtkApp*) user_data;
    GdkRectangle rect = {(int)x, (int)y, 1, 1};
    
    gtk_popover_set_pointing_to (GTK_POPOVER (app->summary_context_menu), &rect);
    gtk_popover_popup (GTK_POPOVER (app->summary_context_menu));
}

// Initialize context menus for all TreeViews
void ugtk_context_menus_init (UgtkApp *app)
{
    GtkGesture *gesture;
    GMenu *menu;
    
    // Install CSS for hover highlighting (one-time)
    ensure_hover_css_installed ();
    
    // Hook hover behavior for ALL popovers inside the menubar (File, Edit, View, etc.)
    // We recursively walk the menubar widget tree and connect to any popover's visibility signal
    popover_walk_widgets (app->menubar.self, (GFunc) hook_menubar_popover, NULL);
    
    // Get the menubar Move To menu reference from the menubar widget's stored data
    GMenuModel *menubar_model = gtk_popover_menu_bar_get_menu_model (GTK_POPOVER_MENU_BAR (app->menubar.self));
    app->menubar_move_to_menu = G_MENU (g_object_get_data (G_OBJECT (menubar_model), "move-to-menu"));
    // Trigger initial population of Move To menu
    ugtk_menubar_sync_category (&app->menubar, app, TRUE);
    
    // Category context menu
    menu = ugtk_create_category_context_menu ();
    app->category_context_menu = gtk_popover_menu_new_from_model (G_MENU_MODEL (menu));
    gtk_widget_set_parent (app->category_context_menu, GTK_WIDGET (app->traveler.category.view));
    gtk_popover_set_has_arrow (GTK_POPOVER (app->category_context_menu), FALSE);
    g_signal_connect (app->category_context_menu, "notify::visible", G_CALLBACK (on_popover_visible_notify), NULL);
    g_object_unref (menu);
    
    gesture = gtk_gesture_click_new ();
    gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), 3);  // Right button
    g_signal_connect (gesture, "pressed", G_CALLBACK (on_category_right_click), app);
    gtk_widget_add_controller (GTK_WIDGET (app->traveler.category.view), GTK_EVENT_CONTROLLER (gesture));
    
    // Download context menu
    menu = ugtk_create_download_context_menu ();
    app->download_context_menu = gtk_popover_menu_new_from_model (G_MENU_MODEL (menu));
    gtk_widget_set_parent (app->download_context_menu, GTK_WIDGET (app->traveler.download.view));
    gtk_popover_set_has_arrow (GTK_POPOVER (app->download_context_menu), FALSE);
    g_signal_connect (app->download_context_menu, "notify::visible", G_CALLBACK (on_popover_visible_notify), NULL);
    g_object_unref (menu);
    
    gesture = gtk_gesture_click_new ();
    gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), 3);  // Right button
    g_signal_connect (gesture, "pressed", G_CALLBACK (on_download_right_click), app);
    gtk_widget_add_controller (GTK_WIDGET (app->traveler.download.view), GTK_EVENT_CONTROLLER (gesture));
    
    // Summary context menu
    menu = ugtk_create_summary_context_menu ();
    app->summary_context_menu = gtk_popover_menu_new_from_model (G_MENU_MODEL (menu));
    gtk_widget_set_parent (app->summary_context_menu, GTK_WIDGET (app->summary.view));
    gtk_popover_set_has_arrow (GTK_POPOVER (app->summary_context_menu), FALSE);
    g_signal_connect (app->summary_context_menu, "notify::visible", G_CALLBACK (on_popover_visible_notify), NULL);
    g_object_unref (menu);
    
    gesture = gtk_gesture_click_new ();
    gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), 3);  // Right button
    g_signal_connect (gesture, "pressed", G_CALLBACK (on_summary_right_click), app);
    gtk_widget_add_controller (GTK_WIDGET (app->summary.view), GTK_EVENT_CONTROLLER (gesture));
}
