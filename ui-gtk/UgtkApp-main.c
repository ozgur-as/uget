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

#define _XOPEN_SOURCE 700
// ----------------------------------------------------------------------------
// Windows

#if defined _WIN32 || defined _WIN64

// This is for ATTACH_PARENT_PROCESS
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x501	// WinXP
#endif

#include <stdio.h>
#include <windows.h>

#if defined _WINDOWS
static void atexit_callback (void)
{
	FreeConsole ();
}

static void win32_console_init (void)
{
	typedef BOOL (CALLBACK* LPFNATTACHCONSOLE) (DWORD);
	LPFNATTACHCONSOLE	AttachConsole;
	HMODULE				hmod;

	// If stdout hasn't been redirected to a file, alloc a console
	//  (_istty() doesn't work for stuff using the GUI subsystem)
	if (_fileno(stdout) == -1 || _fileno(stdout) == -2) {
		AttachConsole = NULL;
#ifdef UNICODE
		if ((hmod = GetModuleHandle(L"kernel32.dll")))
#else
		if ((hmod = GetModuleHandle("kernel32.dll")))
#endif
		{
			AttachConsole = (LPFNATTACHCONSOLE) GetProcAddress(hmod, "AttachConsole");
		}
		if ( (AttachConsole && AttachConsole (ATTACH_PARENT_PROCESS)) || AllocConsole() )
		{
			freopen("CONOUT$", "w", stdout);
			freopen("CONOUT$", "w", stderr);
			atexit (atexit_callback);
		}
	}
}

int WINAPI WinMain (HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArgument,
                    int nFunsterStil)
{
	int	main (int argc, char** argv);	// below main()

	return  main (__argc, __argv);
}
#endif // _WINDOWS

#else
#include <unistd.h>    // sync()
#endif // _WIN32 || _WIN64

// ----------------------------------------------------------------------------

#include <stdlib.h>    // exit(), EXIT_SUCCESS, EXIT_FAILURE
#include <signal.h>    // signal(), SIGTERM
#include <UgUtil.h>
#include <UgFileUtil.h>
#include <UgtkApp.h>

// OpenSSL
#ifdef USE_OPENSSL
#include <UgThread.h>
#include <openssl/crypto.h>

static UgMutex* lockarray;

static void __attribute__((unused)) lock_callback (int mode, int type, char *file, int line)
{
	(void)file;
	(void)line;

	if (mode & CRYPTO_LOCK) {
		ug_mutex_lock (&(lockarray[type]));
	}
	else {
		ug_mutex_unlock (&(lockarray[type]));
	}
}

static unsigned long __attribute__((unused)) thread_id (void)
{
	unsigned long ret;

	ret = (unsigned long) ug_thread_self();
	return(ret);
}

static void  init_locks (void)
{
	int i;

	lockarray = (UgMutex*) OPENSSL_malloc (CRYPTO_num_locks() *
	                                       sizeof(UgMutex));
	for (i=0; i<CRYPTO_num_locks(); i++) {
		ug_mutex_init (&(lockarray[i]));
	}

	CRYPTO_set_id_callback ((unsigned long (*)())thread_id);
	CRYPTO_set_locking_callback ((void (*)())lock_callback);
}

static void kill_locks(void)
{
	int i;

	CRYPTO_set_locking_callback (NULL);
	for (i = 0; i < CRYPTO_num_locks (); i++)
		ug_mutex_clear (&(lockarray[i]));

	OPENSSL_free(lockarray);
}
#endif // USE_OPENSSL

// GnuTLS
#ifdef USE_GNUTLS
#include <gcrypt.h>
#include <errno.h>

GCRY_THREAD_OPTION_PTHREAD_IMPL;

static void init_locks (void)
{
	gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
}

#define kill_locks()
#endif // USE_GNUTLS

// libnotify
#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

// GStreamer
#ifdef HAVE_GSTREAMER
#include <gst/gst.h>
gboolean  gst_inited  = FALSE;
#endif

#include <glib/gi18n.h>

// ----------------------------------------------------------------------------
// SIGTERM
UgtkApp*  ugtk_app;
gboolean  ugtk_quitting = FALSE;

static void sys_signal_handler (int sig)
{
//	void  ug_socket_server_close (UgSocketServer* server);

	switch (sig) {
	case SIGINT:  // Ctrl-C
	case SIGTERM: // termination request
	case SIGABRT: // Abnormal termination (abort)
//	case SIGQUIT:
		// avoid crash when program re-enter signal handler.
		if (ugtk_quitting == FALSE) {
			ugtk_quitting = TRUE;
			// This will quit gtk_main() to main()
			ugtk_app_quit (ugtk_app);
#if !(defined _WIN32 || defined _WIN64)
			sync();
#endif
		}
		break;

//	case SIGSEGV:
//		signal (SIGSEGV, NULL);
//		ug_socket_server_close (ugtk_app->rpc->server);
//		break;

	default:
		break;
	}
}

/*
static void sys_set_sigaction ()
{
    struct sigaction action, old_action;

    action.sa_handler = sys_signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    //action.sa_flags |= SA_RESTART;

    sigaction(SIGINT, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN) {
        sigaction(SIGINT, &action, NULL);
    }
}
*/

// ----------------------------------------------------------------------------
// main ()

int  main (int argc, char** argv)
{
	UgetRpc*  rpc;

	// I18N
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	// Command line
	if (ug_args_find_version (argc-1, argv+1)) {
#if (defined _WIN32 || defined _WIN64) && defined _WINDOWS
		win32_console_init ();
#endif
		g_print ("uGet " PACKAGE_VERSION " for GTK+" "\n");
		return EXIT_SUCCESS;
	}
	if (ug_args_find_help (argc-1, argv+1)) {
#if (defined _WIN32 || defined _WIN64) && defined _WINDOWS
		win32_console_init ();
#endif
		ug_option_entry_print_help (uget_option_entry,
		                            argv[0], "[URL]", NULL);
		return EXIT_SUCCESS;
	}

	// JSON-RPC server
	rpc = uget_rpc_new (NULL);
#ifdef USE_UNIX_DOMAIN_SOCKET
	rpc->backup_dir = ug_build_filename (ugtk_get_config_dir (),
	                                     UGTK_APP_DIR, "RPC-socket", NULL);
	uget_rpc_use_unix_socket (rpc, rpc->backup_dir, -1);
	ug_free (rpc->backup_dir);
#endif
	rpc->backup_dir = g_build_filename (ugtk_get_config_dir (),
	                                    UGTK_APP_DIR, "attachment", NULL);
	ug_create_dir_all (rpc->backup_dir, -1);
	if (uget_rpc_start_server (rpc, TRUE))
		uget_rpc_send_command (rpc, argc-1, argv+1);
	else {
		uget_rpc_send_command (rpc, argc-1, argv+1);
		uget_rpc_free (rpc);
		return EXIT_SUCCESS;
	}

	// GTK+
	// Disable CSD to prefer native Windows titlebar
#if defined _WIN32 || defined _WIN64
	g_setenv ("GTK_CSD", "0", TRUE);
#endif
	gtk_init ();
	// SSL
#if defined USE_GNUTLS || defined USE_OPENSSL
	init_locks ();
#endif
	// libnotify
#ifdef HAVE_LIBNOTIFY
	notify_init ("uGet");
#endif
	// GStreamer
#ifdef HAVE_GSTREAMER
	gst_inited = gst_init_check (&argc, &argv, NULL);
#endif

	ugtk_app = g_malloc0 (sizeof (UgtkApp));
	ugtk_app_init (ugtk_app, rpc);

	// signal handler
	signal (SIGINT,  sys_signal_handler);
	signal (SIGTERM, sys_signal_handler);
	signal (SIGABRT, sys_signal_handler);
//	signal (SIGSEGV, sys_signal_handler);
//	signal (SIGQUIT, sys_signal_handler);

	while (g_list_model_get_n_items (gtk_window_get_toplevels ()) > 0)
		g_main_context_iteration (NULL, TRUE);

	// avoid crash when program re-enter signal handler.
	ugtk_quitting = TRUE;
	// clear/free other resource
	uget_app_clear_attachment ((UgetApp*) ugtk_app);
	ugtk_app_final (ugtk_app);
	g_free (ugtk_app);

	// sleep 2 second to wait thread and shutdown RPC
	g_usleep (1000000);
	uget_rpc_free (rpc);
	g_usleep (1000000);

	// libnotify
#ifdef HAVE_LIBNOTIFY
	if (notify_is_initted ())
		notify_uninit ();
#endif
	// SSL
#if defined USE_GNUTLS || defined USE_OPENSSL
	kill_locks ();
#endif

	return EXIT_SUCCESS;
}

