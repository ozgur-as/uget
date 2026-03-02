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


#ifndef UGTK_PROXY_FORM_H
#define UGTK_PROXY_FORM_H

#include <gtk/gtk.h>
#include <UgetNode.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


typedef struct  UgtkProxyForm        UgtkProxyForm;

struct UgtkProxyForm
{
	GtkWidget*    self;     // top level widget

	GtkWidget*    type;     // GtkDropDown

	// classic
//	struct UgtkProxyFormStd {
	GtkWidget*    std;
	// proxy server
	GtkWidget*    host;     // GtkEntry
	GtkWidget*    port;     // GtkEntry
	// authentication
	GtkWidget*    user;     // GtkEntry
	GtkWidget*    password; // GtkEntry
//	} std;

	// User changed entry
	struct UgtkProxyFormChanged
	{
		gboolean  enable:1;
		// UgProxyFormStd
		gboolean  type:1;
		gboolean  host:1;
		gboolean  port:1;
		gboolean  user:1;
		gboolean  password:1;
	} changed;


//#ifdef HAVE_LIBPWMD
	struct UgtkProxyFormPwmd
	{
		GtkWidget*  self;

		GtkWidget*  socket;
		GtkWidget*  socket_args;
		GtkWidget*  file;
		GtkWidget*  element;

		// User changed entry
		struct UgtkProxyFormPwmdChanged {
			gboolean  socket:1;
			gboolean  socket_args:1;
			gboolean  file:1;
			gboolean  element:1;
		} changed;
	} pwmd;
//#endif	// HAVE_LIBPWMD
};

void  ugtk_proxy_form_init (UgtkProxyForm* pform);
void  ugtk_proxy_form_get  (UgtkProxyForm* pform, UgInfo* info);
void  ugtk_proxy_form_set  (UgtkProxyForm* pform, UgInfo* info, gboolean keep_changed);


#ifdef __cplusplus
}
#endif

#endif  // End of UGTK_PROXY_FORM_H

