static const char	uget_license[] =
{
" Copyright (C) 2005-2020 by C.H. Huang"                                "\n"
" plushuang.tw@gmail.com"                                               "\n"
                                                                        "\n"
"This library is free software; you can redistribute it and/or"         "\n"
"modify it under the terms of the GNU Lesser General Public"            "\n"
"License as published by the Free Software Foundation; either"          "\n"
"version 2.1 of the License, or (at your option) any later version."	"\n"
                                                                        "\n"
"This library is distributed in the hope that it will be useful,"       "\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of"        "\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU"     "\n"
"Lesser General Public License for more details."                       "\n"
                                                                        "\n"
"You should have received a copy of the GNU Lesser General Public"      "\n"
"License along with this library; if not, write to the Free Software"   "\n"
"Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA" "\n"
                                                                        "\n"
"---------"                                                             "\n"
                                                                        "\n"
"In addition, as a special exception, the copyright holders give"       "\n"
"permission to link the code of portions of this program with the"      "\n"
"OpenSSL library under certain conditions as described in each"         "\n"
"individual source file, and distribute linked combinations"            "\n"
"including the two."                                                    "\n"
"You must obey the GNU Lesser General Public License in all respects"   "\n"
"for all of the code used other than OpenSSL.  If you modify"           "\n"
"file(s) with this exception, you may extend this exception to your"    "\n"
"version of the file(s), but you are not obligated to do so.  If you"   "\n"
"do not wish to do so, delete this exception statement from your"       "\n"
"version.  If you delete this exception statement from all source"      "\n"
"files in the program, then also delete it here."                       "\n" "\0"
};

#include <UgtkApp.h>
#include <UgtkAboutDialog.h>

#include <glib/gi18n.h>

#define	UGET_URL_WEBSITE    "http://ugetdm.com/"

// static data
static const gchar*  uget_authors[] = { "C.H. Huang  (\xE9\xBB\x83\xE6\xAD\xA3\xE9\x9B\x84)", NULL };
static const gchar*  uget_artists[] = { "Michael Tunnell (visuex.com)", NULL};
static const gchar*  uget_comments  = N_("Download Manager");
static const gchar*  uget_copyright = "Copyright (C) 2005-2020 C.H. Huang";
static const gchar*  translator_credits = N_("translator-credits");

static gboolean on_close_request (GtkWindow* window, UgtkAboutDialog* adialog)
{
	ugtk_about_dialog_free (adialog);
	return TRUE;
}

UgtkAboutDialog*  ugtk_about_dialog_new (GtkWindow* parent)
{
	UgtkAboutDialog*   adialog;
	char*      path;
	char*      comments;

	adialog = g_malloc (sizeof (UgtkAboutDialog));
	adialog->self = (GtkWindow*) gtk_about_dialog_new ();
	gtk_window_set_transient_for (adialog->self, parent);

	path = g_build_filename (
			ugtk_get_data_dir (), "pixmaps", "uget", "logo.png", NULL);
	adialog->texture = gdk_texture_new_from_filename (path, NULL);
	if (adialog->texture == NULL) {
		// Fallback: source tree path for uninstalled/dev builds
		g_free (path);
		path = g_build_filename (UG_SOURCE_DIR, "pixmaps", "logo.png", NULL);
		adialog->texture = gdk_texture_new_from_filename (path, NULL);
	}
	g_free (path);

	comments = g_strconcat (
			gettext (uget_comments),     "\n",
			_("uGet Founder: "),         uget_authors[0], "\n",
			_("uGet Project Manager: "), uget_artists[0], "\n",
			NULL);

	g_object_set (adialog->self,
			"logo", adialog->texture,
			"program-name", UGTK_APP_NAME,
			"version", PACKAGE_VERSION,
			"comments", comments,
			"copyright", uget_copyright,
#if defined _WIN32 || defined _WIN64
			"website-label", UGET_URL_WEBSITE,
#else
			"website", UGET_URL_WEBSITE,
#endif
			"license", uget_license,
			"authors", uget_authors,
			"artists", uget_artists,
			"translator-credits", gettext (translator_credits),
			NULL);

	g_free (comments);

	g_signal_connect (adialog->self, "close-request",
			G_CALLBACK (on_close_request), adialog);

	return adialog;
}

void ugtk_about_dialog_free (UgtkAboutDialog* adialog)
{
	gtk_window_destroy (adialog->self);
	if (adialog->texture)
		g_object_unref (adialog->texture);
	g_free (adialog);
}

void ugtk_about_dialog_run (UgtkAboutDialog* adialog, const gchar* info_text)
{
	if (info_text)
		g_object_set (adialog->self, "system-information", info_text, NULL);
	gtk_widget_set_visible (GTK_WIDGET (adialog->self), TRUE);
}
