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

#if defined _WIN32 || defined _WIN64
#include <windows.h>
#include <shellapi.h>  // ShellExecuteW()
#endif

#include <UgtkMenubar.h>
#include <UgtkSettingDialog.h>
#include <UgtkConfirmDialog.h>
#include <UgtkAboutDialog.h>
#include <UgtkConfig.h>
#include <UgtkUtil.h>
#include <UgtkApp.h>

#include <glib/gi18n.h>

// ----------------------------------------------------------------------------
// UgtkFileMenu
#if 0
static void on_create_download (GtkWidget* widget, UgtkApp* app)
{
	ugtk_app_create_download (app, NULL, NULL);
}
// ... (all callbacks stubbed)
// ...
static void  on_about (GtkWidget* widget, UgtkApp* app)
{
	UgtkAboutDialog*  adialog;

	adialog = ugtk_about_dialog_new (app->window.self);
//	if (app->update_info.ready)
//		ugtk_about_dialog_set_info (adialog, app->update_info.text->str);
	ugtk_about_dialog_run (adialog);
}
#endif

// ----------------------------------------------------------------------------
// UgtkMenubar

void ugtk_menubar_init_callback (UgtkMenubar* menubar, UgtkApp* app)
{
    // Register GActions for the GtkPopoverMenuBar
    ugtk_menubar_register_actions (app);
    // Initialize right-click context menus for TreeViews
    ugtk_context_menus_init (app);
    (void)menubar; // Avoid unused parameter usage since legacy pointers are gone

// Disable all the old code connections for now since pointers are NULL
#if 0
	// ----------------------------------------------------
	// UgtkFileMenu
//	g_signal_connect (menubar->file.create_download, "activate",
//			G_CALLBACK (on_create_download), app);
	g_signal_connect_swapped (menubar->file.create_category, "activate",
			G_CALLBACK (ugtk_app_create_category), app);

	// ----------------------------------------------------
	// UgtkEditMenu
	g_signal_connect (menubar->edit.clipboard_monitor, "toggled",
			G_CALLBACK (on_clipboard_monitor), app);
	g_signal_connect (menubar->edit.clipboard_quiet, "toggled",
			G_CALLBACK (on_clipboard_quiet), app);
	g_signal_connect (menubar->edit.commandline_quiet, "toggled",
			G_CALLBACK (on_commandline_quiet), app);
	g_signal_connect (menubar->edit.skip_existing, "toggled",
			G_CALLBACK (on_skip_existing), app);
	g_signal_connect (menubar->edit.apply_recent, "toggled",
			G_CALLBACK (on_apply_recent), app);
	// completion action
	g_signal_connect (menubar->edit.completion.disable, "activate",
			G_CALLBACK (on_config_completion), app);
	g_signal_connect (menubar->edit.completion.hibernate, "activate",
			G_CALLBACK (on_config_completion), app);
	g_signal_connect (menubar->edit.completion.suspend, "activate",
			G_CALLBACK (on_config_completion), app);
	g_signal_connect (menubar->edit.completion.shutdown, "activate",
			G_CALLBACK (on_config_completion), app);
	g_signal_connect (menubar->edit.completion.reboot, "activate",
			G_CALLBACK (on_config_completion), app);
	g_signal_connect (menubar->edit.completion.custom, "activate",
			G_CALLBACK (on_config_completion), app);
	g_signal_connect (menubar->edit.completion.remember, "toggled",
			G_CALLBACK (on_config_completion_remember), app);
	g_signal_connect (menubar->edit.completion.help, "activate",
			G_CALLBACK (on_config_completion_help), app);
	// settings
	g_signal_connect (menubar->edit.settings, "activate",
			G_CALLBACK (on_config_settings), app);

	// ----------------------------------------------------
	// UgtkViewMenu
	g_signal_connect (menubar->view.toolbar, "toggled",
			G_CALLBACK (on_change_visible_widget), app);
	g_signal_connect (menubar->view.statusbar, "toggled",
			G_CALLBACK (on_change_visible_widget), app);
	g_signal_connect (menubar->view.category, "toggled",
			G_CALLBACK (on_change_visible_widget), app);
	g_signal_connect (menubar->view.summary, "toggled",
			G_CALLBACK (on_change_visible_widget), app);
	// summary items
	g_signal_connect (menubar->view.summary_items.name, "toggled",
			G_CALLBACK (on_change_visible_summary), app);
	g_signal_connect (menubar->view.summary_items.folder, "toggled",
			G_CALLBACK (on_change_visible_summary), app);
	g_signal_connect (menubar->view.summary_items.category, "toggled",
			G_CALLBACK (on_change_visible_summary), app);
	g_signal_connect (menubar->view.summary_items.uri, "toggled",
			G_CALLBACK (on_change_visible_summary), app);
	g_signal_connect (menubar->view.summary_items.message, "toggled",
			G_CALLBACK (on_change_visible_summary), app);
	// download columns
	g_signal_connect (menubar->view.columns.complete, "toggled",
			G_CALLBACK (on_change_visible_column), app);
	g_signal_connect (menubar->view.columns.total, "toggled",
			G_CALLBACK (on_change_visible_column), app);
	g_signal_connect (menubar->view.columns.percent, "toggled",
			G_CALLBACK (on_change_visible_column), app);
	g_signal_connect (menubar->view.columns.elapsed, "toggled",
			G_CALLBACK (on_change_visible_column), app);
	g_signal_connect (menubar->view.columns.left, "toggled",
			G_CALLBACK (on_change_visible_column), app);
	g_signal_connect (menubar->view.columns.speed, "toggled",
			G_CALLBACK (on_change_visible_column), app);
	g_signal_connect (menubar->view.columns.upload_speed, "toggled",
			G_CALLBACK (on_change_visible_column), app);
	g_signal_connect (menubar->view.columns.uploaded, "toggled",
			G_CALLBACK (on_change_visible_column), app);
	g_signal_connect (menubar->view.columns.ratio, "toggled",
			G_CALLBACK (on_change_visible_column), app);
	g_signal_connect (menubar->view.columns.retry, "toggled",
			G_CALLBACK (on_change_visible_column), app);
	g_signal_connect (menubar->view.columns.category, "toggled",
			G_CALLBACK (on_change_visible_column), app);
	g_signal_connect (menubar->view.columns.uri, "toggled",
			G_CALLBACK (on_change_visible_column), app);
	g_signal_connect (menubar->view.columns.added_on, "toggled",
			G_CALLBACK (on_change_visible_column), app);
	g_signal_connect (menubar->view.columns.completed_on, "toggled",
			G_CALLBACK (on_change_visible_column), app);

	// ----------------------------------------------------
	// UgtkCategoryMenu
	g_signal_connect_swapped (menubar->category.create, "activate",
			G_CALLBACK (ugtk_app_create_category), app);
	g_signal_connect (menubar->category.delete, "activate",
			G_CALLBACK (on_delete_category), app);
	g_signal_connect_swapped (menubar->category.properties, "activate",
			G_CALLBACK (ugtk_app_edit_category), app);
	g_signal_connect (menubar->category.move_up, "activate",
			G_CALLBACK (on_move_category_up), app);
	g_signal_connect (menubar->category.move_down, "activate",
			G_CALLBACK (on_move_category_down), app);

	// ----------------------------------------------------
	// UgtkDownloadMenu
	g_signal_connect (menubar->download.create, "activate",
			G_CALLBACK (on_create_download), app);
	g_signal_connect (menubar->download.delete, "activate",
			G_CALLBACK (on_delete_download), app);
	// file & folder
	g_signal_connect (menubar->download.delete_file, "activate",
			G_CALLBACK (on_delete_download_file), app);
	g_signal_connect (menubar->download.open, "activate",
			G_CALLBACK (on_open_download_file), app);
	g_signal_connect (menubar->download.open_folder, "activate",
			G_CALLBACK (on_open_download_folder), app);
	// change status
	g_signal_connect (menubar->download.force_start, "activate",
			G_CALLBACK (on_set_download_force_start), app);
	g_signal_connect (menubar->download.runnable, "activate",
			G_CALLBACK (on_set_download_runnable), app);
	g_signal_connect_swapped (menubar->download.pause, "activate",
			G_CALLBACK (ugtk_app_pause_download), app);
	// move
	g_signal_connect_swapped (menubar->download.move_up, "activate",
			G_CALLBACK (ugtk_app_move_download_up), app);
	g_signal_connect_swapped (menubar->download.move_down, "activate",
			G_CALLBACK (ugtk_app_move_download_down), app);
	g_signal_connect_swapped (menubar->download.move_top, "activate",
			G_CALLBACK (ugtk_app_move_download_top), app);
	g_signal_connect_swapped (menubar->download.move_bottom, "activate",
			G_CALLBACK (ugtk_app_move_download_bottom), app);
	// prioriy
	g_signal_connect (menubar->download.prioriy.high, "activate",
			G_CALLBACK (on_set_download_prioriy), app);
	g_signal_connect (menubar->download.prioriy.normal, "activate",
			G_CALLBACK (on_set_download_prioriy), app);
	g_signal_connect (menubar->download.prioriy.low, "activate",
			G_CALLBACK (on_set_download_prioriy), app);
	// properties
	g_signal_connect_swapped (menubar->download.properties, "activate",
			G_CALLBACK (ugtk_app_edit_download), app);

	// ----------------------------------------------------
	// UgtkHelpMenu
	g_signal_connect (menubar->help.help_online, "activate",
			G_CALLBACK (on_help_online), app);
	g_signal_connect (menubar->help.documentation, "activate",
			G_CALLBACK (on_documentation), app);
	g_signal_connect (menubar->help.support_forum, "activate",
			G_CALLBACK (on_support_forum), app);
	g_signal_connect (menubar->help.submit_feedback, "activate",
			G_CALLBACK (on_submit_feedback), app);
	g_signal_connect (menubar->help.report_bug, "activate",
			G_CALLBACK (on_report_bug), app);
	g_signal_connect (menubar->help.keyboard_shortcuts, "activate",
			G_CALLBACK (on_keyboard_shortcuts), app);
	g_signal_connect (menubar->help.check_updates, "activate",
			G_CALLBACK (on_check_updates), app);
	g_signal_connect (menubar->help.about_uget, "activate",
			G_CALLBACK (on_about), app);
#endif
}

void  ugtk_menubar_sync_category (UgtkMenubar* menubar, UgtkApp* app, gboolean reset)
{
	(void)menubar;  // Not using legacy menubar struct
	
	// Populate menubar Move To menu with categories
	if (app->menubar_move_to_menu) {
		// Clear existing items
		g_menu_remove_all (app->menubar_move_to_menu);
		
		// Add category items
		UgetNode *cnode = app->real.children;
		gint index = 0;
		while (cnode) {
			UgetCommon *common = ug_info_get (cnode->info, UgetCommonInfo);
			const gchar *name = (common && common->name) ? common->name : _("Unnamed");
			
			// Create menu item with action target (category index)
			GMenuItem *item = g_menu_item_new (name, NULL);
			g_menu_item_set_action_and_target (item, "win.move-to-category", "i", index);
			g_menu_append_item (app->menubar_move_to_menu, item);
			g_object_unref (item);
			
			cnode = cnode->next;
			index++;
		}
	}
}

