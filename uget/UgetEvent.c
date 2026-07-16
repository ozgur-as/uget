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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdarg.h>    // va_list, va_start(), va_end()
#include <UgString.h>
#include <UgJson-custom.h>
#include <UgetEvent.h>

#ifdef HAVE_GLIB
#include <glib.h>      // g_malloc/g_free
#include <glib/gi18n.h>
#else
#define N_(x)   x
#endif

// UGET_EVENT_NORMAL
static const char*	normal_msg[] =
{
	NULL,                                                       // UGET_EVENT_NORMAL_CUSTOM
	N_("Connecting..."),                                        // UGET_EVENT_NORMAL_CONNECT
	N_("Transmitting..."),                                      // UGET_EVENT_NORMAL_TRANSMIT,
	N_("Retry"),                                                // UGET_EVENT_NORMAL_RETRY,
	N_("Download completed"),                                   // UGET_EVENT_NORMAL_COMPLETE,
	N_("Finished"),                                             // UGET_EVENT_NORMAL_FINISH,
	// resumable
	N_("Resumable"),                                            // UGET_EVENT_NORMAL_RESUMABLE,
	N_("Not Resumable"),                                        // UGET_EVENT_NORMAL_NOT_RESUMABLE,
};
static const int  n_normal_msg = sizeof (normal_msg) / sizeof (char*);

// UGET_EVENT_WARNING
static const char* warning_msg[] =
{
	NULL,                                                       // UGET_EVENT_WARNING_CUSTOM
	N_("Output file can't be renamed."),                        // UGET_EVENT_WARNING_FILE_RENAME_FAILED
};
static const int  n_warning_msg = sizeof (warning_msg) / sizeof (char*);

// UGET_EVENT_ERROR
static const char*  error_msg[] =
{
	NULL,                                                       // UGET_EVENT_ERROR_CUSTOM
	N_("couldn't connect to host."),                            // UGET_EVENT_ERROR_CONNECT_FAILED
	N_("Folder can't be created."),                             // UGET_EVENT_ERROR_FOLDER_CREATE_FAILED
	N_("File can't be created (bad filename or file exist)."),  // UGET_EVENT_ERROR_FILE_CREATE_FAILED
	N_("File can't be opened."),                                // UGET_EVENT_ERROR_FILE_OPEN_FAILED
	N_("Unable to create thread."),                             // UGET_EVENT_ERROR_THREAD_CREATE_FAILED,
	N_("Incorrect source (different file size)."),              // UGET_EVENT_ERROR_INCORRECT_SOURCE,
	N_("Out of resource (disk full or run out of memory)."),    // UGET_EVENT_ERROR_OUT_OF_RESOURCE
	N_("No output file."),                                      // UGET_EVENT_ERROR_NO_OUTPUT_FILE
	N_("No output setting."),                                   // UGET_EVENT_ERROR_NO_OUTPUT_SETTING
	N_("Too many retries."),                                    // UGET_EVENT_ERROR_TOO_MANY_RETRIES
	N_("Unsupported scheme (protocol)."),                       // UGET_EVENT_ERROR_UNSUPPORTED_SCHEME
	N_("Unsupported file."),                                    // UGET_EVENT_ERROR_UNSUPPORTED_FILE
	N_("post file not found."),                                 // UGET_EVENT_ERROR_POST_FILE_NOT_FOUND
	N_("cookie file not found."),                               // UGET_EVENT_ERROR_COOKIE_FILE_NOT_FOUND
};
static const int  n_error_msg = sizeof (error_msg) / sizeof (char*);

// extern
const UgEntry  UgetEventEntry[] =
{
	{"string", offsetof (UgetEvent, string), UG_ENTRY_STRING, NULL, NULL},
	{"type",   offsetof (UgetEvent, type),   UG_ENTRY_INT,    NULL, NULL},
	{"time",   offsetof (UgetEvent, time),   UG_ENTRY_CUSTOM, ug_json_parse_time_t, ug_json_write_time_t},
	{NULL}    // null-terminated
};

UgetEvent* uget_event_new (UgetEventType type, ...)
{
	UgetEvent* event;
	va_list    arg_list;

#ifdef HAVE_GLIB
	event = g_malloc0 (sizeof (UgetEvent));
#else
	event = ug_malloc0 (sizeof (UgetEvent));
#endif
	event->self = event;
	event->time = time (NULL);
	event->type = type;
	event->string = NULL;

	va_start (arg_list, type);
	switch (type) {
	case UGET_EVENT_ERROR:
		event->value.code = va_arg (arg_list, int);
		event->string = va_arg (arg_list, char*);
		if (event->string)
			event->string = ug_strdup (event->string);
		else if (event->value.code < n_error_msg) {
#ifdef HAVE_GLIB
			event->string = ug_strdup (gettext (error_msg[event->value.code]));
#else
			event->string = ug_strdup (error_msg[event->value.code]);
#endif // HAVE_GLIB
		}
		break;

	case UGET_EVENT_NORMAL:
		event->value.code = va_arg (arg_list, int);
		event->string = va_arg (arg_list, char*);
		if (event->string)
			event->string = ug_strdup (event->string);
		else if (event->value.code < n_normal_msg) {
#ifdef HAVE_GLIB
			event->string = ug_strdup (gettext (normal_msg[event->value.code]));
#else
			event->string = ug_strdup (normal_msg[event->value.code]);
#endif // HAVE_GLIB
		}
		break;

	case UGET_EVENT_WARNING:
		event->value.code = va_arg (arg_list, int);
		event->string = va_arg (arg_list, char*);
		if (event->string)
			event->string = ug_strdup (event->string);
		else if (event->value.code < n_warning_msg) {
#ifdef HAVE_GLIB
			event->string = ug_strdup (gettext (warning_msg[event->value.code]));
#else
			event->string = ug_strdup (warning_msg[event->value.code]);
#endif // HAVE_GLIB
		}
		break;

	default:
		break;
	}
	va_end (arg_list);

	return event;
}

void uget_event_free (UgetEvent* event)
{
	ug_free (event->string);
#ifdef HAVE_GLIB
	g_free (event);
#else
	ug_free (event);
#endif
}

