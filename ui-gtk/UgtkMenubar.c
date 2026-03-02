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
// UgtkMenubar

void ugtk_menubar_init_callback (UgtkMenubar* menubar, UgtkApp* app)
{
    ugtk_menubar_register_actions (app);
    ugtk_context_menus_init (app);
    (void)menubar;
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

