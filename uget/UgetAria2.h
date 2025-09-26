/*
 *
 *   Copyright (C) 2011-2020 by C.H. Huang
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

#ifndef UGET_ARIA2_H
#define UGET_ARIA2_H

#include <glib.h>

#include <UgRegistry.h>
#include <UgSLink.h>
#include <UgThread.h>
#include <UgJsonrpc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UgetAria2          UgetAria2;
typedef struct UgetAria2Thread    UgetAria2Thread;

typedef enum {
	UGET_ARIA2_ERROR_NONE,
	UGET_ARIA2_ERROR_RPC,
	UGET_ARIA2_ERROR_LAUNCH,
} UGET_ARIA2_ERROR;

struct UgetAria2
{
	int       ref_count;

        UgMutex              mutex;
        UgetAria2Thread*     worker;

        GThread*             thread;
        GMutex               thread_lock;
        GCond                thread_cond;
        volatile gint        stop;

	// request -> global thread -> requested + responsed
	//
	// requested + responsed -+-> reuse
	//                        |      |
	//                        |      V
	//                        +-> recycled
	//

	// JSON objects(UgJsonrpcObject) pair
	UgJsonrpcArray   queuing;
	UgJsonrpcArray   recycled;
	// completed
	UgSLinks         requested;
	UgSLinks         responsed;
	UgMutex          completed_mutex;
	int              completed_changed;
	// common data for status request
	UgValue          status_keys;

	unsigned int  error;
	unsigned int  batch_len;
	unsigned int  batch_additional;
	unsigned int  polling_interval;

	// boolean
	uint8_t       connect_fail:1;
	uint8_t       speed_required:1;
	uint8_t       limit_required:1;
	uint8_t       launched:1;
	uint8_t       shutdown:1;
	uint8_t       uri_changed:1;
	uint8_t       uri_remote:1;

	char*     uri;
	char*     path;
	char*     args;
	char*     token;  // --rpc-secret=<TOKEN>

	struct {
		int   download;
		int   upload;
	} speed, limit;
	// speed limit counter
	int       limit_count_prev;
	int       limit_count;
};

UgetAria2* uget_aria2_new (void);
void uget_aria2_ref (UgetAria2* ua2);
void uget_aria2_unref (UgetAria2* ua2);
void uget_aria2_free (UgetAria2* ua2);

void uget_aria2_start_thread (UgetAria2* uaria2);
void uget_aria2_stop_thread  (UgetAria2* uaria2);

void uget_aria2_set_uri  (UgetAria2* uaria2, const char* uri);
void uget_aria2_set_path (UgetAria2* uaria2, const char* path);
void uget_aria2_set_args (UgetAria2* uaria2, const char* args);
void uget_aria2_set_token (UgetAria2* uaria2, const char* token);
void uget_aria2_set_speed (UgetAria2* uaria2, int dl_speed, int ul_speed);

int  uget_aria2_launch   (UgetAria2* aria2);
void uget_aria2_shutdown (UgetAria2* aria2);

// If you want to get response, set request->id.type = UG_VALUE_INT
// If you don't need response, set request->id.type = UG_VALUE_NONE
UgJsonrpcObject*  uget_aria2_alloc   (UgetAria2* aria2, int is_request, int has_response);
void              uget_aria2_request (UgetAria2* aria2, UgJsonrpcObject* request);
UgJsonrpcObject*  uget_aria2_respond (UgetAria2* aria2, UgJsonrpcObject* request);
void              uget_aria2_recycle (UgetAria2* aria2, UgJsonrpcObject* jobject);
UgValue*          uget_aria2_clear_token (UgJsonrpcObject* jobject);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // End of UGET_ARIA2_H
