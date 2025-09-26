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

#include <UgString.h>
#include <UgUri.h>
#include <UgUtil.h>
#include <UgJsonrpcCurl.h>
#include <UgetAria2.h>
#include <curl/curl.h>

#if defined _WIN32 || defined _WIN64
#include <windows.h>
#include <shellapi.h>  // ShellExecuteW()
#else
#include <unistd.h>    // fork(), execlp()
#include <fcntl.h>
#include <errno.h>
#endif // _WIN32 || _WIN64

#define RPC_URI              "http://localhost:6800/jsonrpc"
#define RPC_BATCH_LEN        5
#define RPC_INTERVAL         500
#define ARIA2_PATH           "aria2c"
#define ARIA2_ARGS           "--enable-rpc=true -D --check-certificate=false"

#if defined _WIN32 || defined _WIN64
#define  ug_sleep                 Sleep
#else
#define  ug_sleep(millisecond)    usleep (millisecond * 1000)
#endif

#ifdef _MSC_VER
#define strcasecmp   stricmp
#define strncasecmp  strnicmp
#endif

// ----------------------------------------------------------------------------
// UgetAria2Thread

struct UgetAria2Thread
{
	UgThread         thread;
	UgetAria2*       uaria2;
	UgJsonrpcArray   queuing;
	UgJsonrpcArray   request;
	UgJsonrpcArray   response;
	UgJsonrpcCurl    json;
	int              finalized;
};

static UgThreadResult  uget_aria2_thread (UgetAria2Thread* uathread);

static UgetAria2Thread* uget_aria2_thread_new (UgetAria2* uaria2)
{
	UgetAria2Thread*  uat;
	UgThread          thread;

	uat = ug_malloc (sizeof (UgetAria2Thread));
	uat->uaria2 = uaria2;
	ug_jsonrpc_array_init (&uat->queuing, 16);
	ug_jsonrpc_array_init (&uat->request, 16);
	ug_jsonrpc_array_init (&uat->response, 16);
	ug_jsonrpc_curl_init (&uat->json);
	ug_jsonrpc_curl_set_url (&uat->json, uaria2->uri);
	uat->finalized = FALSE;

	uget_aria2_ref (uaria2);
	ug_thread_create (&thread, (UgThreadFunc) uget_aria2_thread, uat);
	ug_thread_unjoin (&thread);

	return uat;
}

static void uget_aria2_thread_free (UgetAria2Thread* uat)
{
	ug_jsonrpc_array_clear (&uat->queuing, TRUE);
	ug_jsonrpc_array_clear (&uat->request, TRUE);
	ug_jsonrpc_array_clear (&uat->response, TRUE);
	ug_jsonrpc_curl_final (&uat->json);
	ug_free (uat);
}

static int  uget_aria2_thread_queuing (UgetAria2Thread* uathread)
{
	UgetAria2*  uaria2;
	int  length;
	int  index;

	index = 0;
	uaria2 = uathread->uaria2;
	length = uathread->queuing.length;

	ug_mutex_lock (&uaria2->mutex);
	ug_array_alloc (&uathread->queuing, uaria2->queuing.length);
	while (index < uaria2->queuing.length)
		uathread->queuing.at[length++] = uaria2->queuing.at[index++];
	uaria2->queuing.length = 0;
	ug_mutex_unlock (&uaria2->mutex);

	return length;
}

static void  uget_aria2_match_response (UgetAria2Thread* uathread)
{
	UgetAria2*        uaria2;
	UgJsonrpcObject*  req;
	UgJsonrpcObject*  res;
	int   length;
	int   index_req, index_res;

	uaria2 = uathread->uaria2;
	length = uathread->request.length;

	for (index_req = 0;  index_req < length;  index_req++) {
		req = uathread->request.at[index_req];
		res = ug_jsonrpc_array_find (&uathread->response,
		                             &req->id, &index_res);
		if (res)
			uathread->response.at[index_res] = NULL;
		uathread->request.at[index_req] = NULL;

		if (req->id.type == UG_VALUE_NONE) {
			uget_aria2_recycle (uaria2, req);
			uget_aria2_recycle (uaria2, res);
		}
		else {
			ug_mutex_lock (&uaria2->completed_mutex);
			ug_slinks_add (&uaria2->requested, req);
			ug_slinks_add (&uaria2->responsed, res);
			ug_mutex_unlock (&uaria2->completed_mutex);
		}
	}
	uathread->request.length = 0;
	uaria2->completed_changed++;

	length = uathread->response.length;
	for (index_res = 0;  index_res < length;  index_res++) {
		res = uathread->response.at[index_res];
		if (res)
			uget_aria2_recycle (uaria2, res);
	}
	uathread->response.length = 0;
}

static int  uget_aria2_thread_request (UgetAria2Thread* uathread)
{
	UgJsonrpcObject*  ujobj;
	UgetAria2*        uaria2;
	int  index;
	int  length;
	int  limit;
	int  counts;

	uaria2 = uathread->uaria2;
	limit  = uaria2->batch_len + uaria2->batch_additional;
	for (index = 0;  index < uathread->queuing.length;  index++) {
		length = uathread->queuing.length - index;
		if (length > limit)
			length = limit;
		ug_array_alloc (&uathread->request, length);

		for (counts = 0;  counts < length;  counts++) {
			ujobj = uathread->queuing.at[index++];
			uathread->request.at[counts] = ujobj;
			if (ujobj->id.type != UG_VALUE_NONE) {
				ujobj = uget_aria2_alloc (uaria2, FALSE, FALSE);
				*(void**) ug_array_alloc (&uathread->response, 1) = ujobj;
			}
		}

		// JSON-RPC
		counts = ug_jsonrpc_call_batch (&uathread->json.rpc,
				&uathread->request, &uathread->response);
		if (counts == -1) {
			uaria2->error = TRUE;
			uaria2->connect_fail = TRUE;
		}

		uget_aria2_match_response (uathread);
	}
	uathread->queuing.length = 0;

	return index;
}

static UgJsonrpcObject*  add_limit_request (UgetAria2* uaria2)
{
	UgJsonrpcObject*  jobj;
	UgValue*          vobj;
	UgValue*          value;

	jobj = uget_aria2_alloc (uaria2, TRUE, TRUE);
	jobj->method_static = "aria2.changeGlobalOption";
	if (jobj->params.type == UG_VALUE_NONE)
		ug_value_init_array (&jobj->params, 2);
	vobj = ug_value_alloc (&jobj->params, 1);
	ug_value_init_object (vobj, 2);

	value = ug_value_alloc (vobj, 1);
	value->name = "max-overall-download-limit";
	value->type = UG_VALUE_STRING;
	value->c.string = ug_strdup_printf ("%dK", uaria2->limit.download / 1024);
	value = ug_value_alloc (vobj, 1);
	value->name = "max-overall-upload-limit";
	value->type = UG_VALUE_STRING;
	value->c.string = ug_strdup_printf ("%dK", uaria2->limit.upload / 1024);
	// max-concurrent-downloads must >= 1
//	value = ug_value_alloc (vobj, 1);
//	value->name = "max-concurrent-downloads";
//	value->type = UG_VALUE_STRING;
//	value->c.string = ug_strdup_printf ("%u", uaria2->limit.connections);

	uget_aria2_request (uaria2, jobj);
	return jobj;
}

static void recycle_limit_request (UgetAria2* uaria2, UgJsonrpcObject* jreq)
{
	UgJsonrpcObject*  jres;

	jres = uget_aria2_respond (uaria2, jreq);
	ug_value_foreach (&jreq->params, ug_value_set_name, NULL);
	uget_aria2_recycle (uaria2, jreq);
	uget_aria2_recycle (uaria2, jres);
}

static UgJsonrpcObject*  add_speed_request (UgetAria2* uaria2)
{
	UgJsonrpcObject*  jobj;

	jobj = uget_aria2_alloc (uaria2, TRUE, TRUE);
	jobj->method_static = "aria2.getGlobalStat";

	uget_aria2_request (uaria2, jobj);
	return jobj;
}

static void  recycle_speed_request (UgetAria2* uaria2, UgJsonrpcObject* jreq)
{
	UgJsonrpcObject*  jres;
	UgValue*          value;

	jres = uget_aria2_respond (uaria2, jreq);
	if (jres && jres->error.code == 0) {
		ug_value_sort_name (&jres->result);
		value = ug_value_find_name (&jres->result, "downloadSpeed");
		uaria2->speed.download = ug_value_get_int (value);
		value = ug_value_find_name (&jres->result, "uploadSpeed");
		uaria2->speed.upload = ug_value_get_int (value);
	}
	uget_aria2_recycle (uaria2, jreq);
	uget_aria2_recycle (uaria2, jres);
}

static UgThreadResult  uget_aria2_thread (UgetAria2Thread* uathread)
{
	UgetAria2*       uaria2;
	UgJsonrpcObject* jreq = NULL;
	UgJsonrpcObject* jobj = NULL;
	UgJsonrpcObject* jreq_shutdown = NULL;
	int  counts;

	uaria2 = uathread->uaria2;
//	temp.index = ug_jsonrpc_array_find_ptr (uaria2);

	for (counts = 0;  ;  counts++) {
		// finalize
		if (uathread->finalized == TRUE) {
			// shutdown request
			if (uaria2->shutdown && jreq_shutdown == NULL) {
				jreq_shutdown = uget_aria2_alloc (uaria2, TRUE, TRUE);
				jreq_shutdown->method_static = "aria2.shutdown";
				uget_aria2_request (uaria2, jreq_shutdown);
			}
			if (uaria2->queuing.length == 0)
				break;
		}
		// URI changed
		ug_mutex_lock (&uaria2->mutex);
		if (uaria2->uri_changed) {
			uaria2->uri_changed = FALSE;
			ug_jsonrpc_curl_set_url (&uathread->json, uaria2->uri);
		}
		ug_mutex_unlock (&uaria2->mutex);

		// additional request
		if (uaria2->limit_count_prev != uaria2->limit_count) {
			uaria2->limit_count_prev  = uaria2->limit_count;
			uaria2->limit_required = TRUE;
			jreq = add_limit_request (uaria2);
			uaria2->batch_additional++;
		}
		if (uaria2->speed_required && (counts & 2) == 2) {
			jobj = add_speed_request (uaria2);
			uaria2->batch_additional++;
		}

		// get requests from queue
		if (uget_aria2_thread_queuing (uathread) == 0) {
			// default: sleep 0.5 second
			ug_sleep (uaria2->polling_interval);
			continue;
		}

		// send requests & get responses
		uget_aria2_thread_request (uathread);

		// recycle additional request
		if (uaria2->limit_required) {
			uaria2->limit_required = FALSE;
			recycle_limit_request (uaria2, jreq);
			uaria2->batch_additional--;
		}
		if (uaria2->speed_required && (counts & 2) == 2) {
			recycle_speed_request (uaria2, jobj);
			uaria2->batch_additional--;
		}
	}

	// shutdown response
	if (jreq_shutdown) {
		jobj = uget_aria2_respond (uaria2, jreq_shutdown);
		uget_aria2_recycle (uaria2, jreq_shutdown);
		uget_aria2_recycle (uaria2, jobj);
	}

	uget_aria2_thread_free (uathread);
	uget_aria2_unref (uaria2);
	return UG_THREAD_RESULT;
}

// ----------------------------------------------------------------------------
// UgetAria2

UgetAria2* uget_aria2_new (void)
{
	UgetAria2*    uaria2;
	UgValue*      value;
	UgValue*      keys;

#if defined _WIN32 || defined _WIN64
	WSADATA WSAData;
	WSAStartup (MAKEWORD (2, 2), &WSAData);
#endif // _WIN32 || _WIN64
	curl_global_init (CURL_GLOBAL_ALL);

	uaria2 = ug_malloc0 (sizeof (UgetAria2));
	uaria2->ref_count = 1;
	uaria2->batch_len = RPC_BATCH_LEN;
	uaria2->polling_interval = RPC_INTERVAL;
	uaria2->speed_required = FALSE;
	uaria2->limit_required = FALSE;
	uaria2->uri = ug_strdup (RPC_URI);
	uaria2->path = ug_strdup (ARIA2_PATH);
	uaria2->args = ug_strdup (ARIA2_ARGS);
	ug_mutex_init (&uaria2->mutex);
	ug_mutex_init (&uaria2->completed_mutex);

	ug_jsonrpc_array_init (&uaria2->queuing,  16);
	ug_jsonrpc_array_init (&uaria2->recycled, 16);
	ug_slinks_init (&uaria2->requested, 16);
	ug_slinks_init (&uaria2->responsed, 16);
	uaria2->completed_changed = 0;

	ug_value_init_array (&uaria2->status_keys, 16);
	keys = &uaria2->status_keys;
	value = ug_value_alloc (keys, 1);
	value->type = UG_VALUE_STRING;
	value->c.string = "status";
	value = ug_value_alloc (keys, 1);
	value->type = UG_VALUE_STRING;
	value->c.string = "totalLength";
	value = ug_value_alloc (keys, 1);
	value->type = UG_VALUE_STRING;
	value->c.string = "completedLength";
	value = ug_value_alloc (keys, 1);
	value->type = UG_VALUE_STRING;
	value->c.string = "uploadLength";
	value = ug_value_alloc (keys, 1);
	value->type = UG_VALUE_STRING;
	value->c.string = "downloadSpeed";
	value = ug_value_alloc (keys, 1);
	value->type = UG_VALUE_STRING;
	value->c.string = "uploadSpeed";
	value = ug_value_alloc (keys, 1);
	value->type = UG_VALUE_STRING;
	value->c.string = "errorCode";
	value = ug_value_alloc (keys, 1);
	value->type = UG_VALUE_STRING;
	value->c.string = "files";
	value = ug_value_alloc (keys, 1);
	value->type = UG_VALUE_STRING;
	value->c.string = "followedBy";

	return uaria2;
}

void uget_aria2_ref (UgetAria2* uaria2)
{
	uaria2->ref_count++;
}

static void
uget_aria2_dispose (UgetAria2* uaria2)
{
        int index;

        if (uaria2 == NULL)
                return;

        ug_mutex_lock (&uaria2->mutex);
        for (index = 0; index < uaria2->queuing.length; index++)
                if (uaria2->queuing.at[index])
                        ug_jsonrpc_object_free (uaria2->queuing.at[index]);
        ug_jsonrpc_array_clear (&uaria2->queuing, 0);

        for (index = 0; index < uaria2->recycled.length; index++)
                if (uaria2->recycled.at[index])
                        ug_jsonrpc_object_free (uaria2->recycled.at[index]);
        ug_jsonrpc_array_clear (&uaria2->recycled, 0);
        ug_mutex_unlock (&uaria2->mutex);

        ug_slinks_foreach (&uaria2->requested, (void*)ug_jsonrpc_object_free, NULL);
        ug_slinks_foreach (&uaria2->responsed, (void*)ug_jsonrpc_object_free, NULL);
        ug_slinks_final (&uaria2->requested);
        ug_slinks_final (&uaria2->responsed);

        ug_value_foreach (&uaria2->status_keys, ug_value_set_string, NULL);
        ug_value_clear (&uaria2->status_keys);

        ug_mutex_clear (&uaria2->completed_mutex);
        ug_mutex_clear (&uaria2->mutex);

        ug_free (uaria2->token);
        ug_free (uaria2->uri);
        ug_free (uaria2->path);
        ug_free (uaria2->args);
        ug_free (uaria2);

        curl_global_cleanup ();
#if defined _WIN32 || defined _WIN64
        WSACleanup ();
#endif // _WIN32 || _WIN64
}

void uget_aria2_unref (UgetAria2* uaria2)
{
        if (uaria2 == NULL)
                return;

        if (--uaria2->ref_count == 0)
                uget_aria2_dispose (uaria2);
}

void uget_aria2_free (UgetAria2* uaria2)
{
        if (uaria2 == NULL)
                return;

        uaria2->ref_count = 1;
        uget_aria2_unref (uaria2);
}

void uget_aria2_start_thread (UgetAria2* uaria2)
{
	if (uaria2->thread == NULL)
		uaria2->thread = uget_aria2_thread_new (uaria2);
}

void uget_aria2_stop_thread (UgetAria2* uaria2)
{
	uaria2->thread->finalized = TRUE;
	uaria2->thread = NULL;
}

void uget_aria2_set_uri (UgetAria2* uaria2, const char* uri)
{
	UgUri  upart;
	int    len;

	if (strcmp (uaria2->uri, uri) != 0) {
		ug_uri_init (&upart, uri);
		uaria2->uri_remote = TRUE;
		if (upart.host != -1) {
			len = ug_uri_host (&upart, NULL);
			// localhost
			// IPv6 "::1"
			// IPv4 "127.0.0.1"
			if (strncmp (uri + upart.host, "::1", len)           == 0 ||
			    strncmp (uri + upart.host, "127.0.0.1", len)     == 0 ||
			    strncasecmp (uri + upart.host, "localhost", len) == 0)
			{
				uaria2->uri_remote = FALSE;
			}
		}
		ug_mutex_lock (&uaria2->mutex);
		ug_free (uaria2->uri);
		uaria2->uri = ug_strdup (uri);
		uaria2->uri_changed = TRUE;
		ug_mutex_unlock (&uaria2->mutex);
	}
}

void uget_aria2_set_path (UgetAria2* uaria2, const char* path)
{
//	ug_mutex_lock (&uaria2->mutex);
	ug_free (uaria2->path);
	uaria2->path = (path) ? ug_strdup (path) : NULL;
//	ug_mutex_unlock (&uaria2->mutex);
}

void uget_aria2_set_args (UgetAria2* uaria2, const char* args)
{
//	ug_mutex_lock (&uaria2->mutex);
	ug_free (uaria2->args);
	uaria2->args = (args) ? ug_strdup (args) : NULL;
//	ug_mutex_unlock (&uaria2->mutex);
}

void uget_aria2_set_token (UgetAria2* uaria2, const char* token)
{
	ug_mutex_lock (&uaria2->mutex);
	ug_free (uaria2->token);
	uaria2->token = (token) ? ug_strdup (token) : NULL;
	ug_mutex_unlock (&uaria2->mutex);
}

void uget_aria2_set_speed (UgetAria2* uaria2, int dl_speed, int ul_speed)
{
	uaria2->limit.download = dl_speed;
	uaria2->limit.upload = ul_speed;
	uaria2->limit_count++;
}

#ifdef __ANDROID__

#if 1  // __ANDROID__

int  uget_aria2_launch (UgetAria2* uaria2)
{
	return uaria2->launched;
}

#else
typedef struct {
	UgetAria2* uaria2;
	char       cmd[1];
} Aria2LaunchData;

static UgThreadResult  aria2_launch_thread (Aria2LaunchData* uald)
{
	int  result;

	result = system (uald->cmd);
	if (result == -1)
		uald->uaria2->launched = FALSE;
	else
		uald->uaria2->launched = TRUE;
	ug_free (uald);
	return UG_THREAD_RESULT;
}

int  uget_aria2_launch (UgetAria2* uaria2)
{
	Aria2LaunchData* uald;
	UgThread  thread;

	uald = ug_malloc (sizeof (Aria2LaunchData) +
			strlen (uaria2->path) + strlen (uaria2->args) + 1);
	uald->uaria2 = uaria2;
	uald->cmd[0] = 0;
	strcat (uald->cmd, uaria2->path);
	strcat (uald->cmd, " ");
	strcat (uald->cmd, uaria2->args);
	ug_thread_create (&thread, (UgThreadFunc)aria2_launch_thread, uald);
	ug_thread_unjoin (&thread);
	return uaria2->launched;
}
#endif  // __ANDROID__

#elif (defined _WIN32 || defined _WIN64)

int  uget_aria2_launch (UgetAria2* uaria2)
{
	uint16_t*  path_utf16;
	uint16_t*  args_utf16;
	int        result;

	if (uaria2->launched == TRUE)
		return TRUE;
	if (uaria2->path == NULL || uaria2->args == NULL)
		return FALSE;

//	ug_mutex_lock (&uaria2->mutex);
	path_utf16 = ug_utf8_to_utf16 (uaria2->path, -1, NULL);
	args_utf16 = ug_utf8_to_utf16 (uaria2->args, -1, NULL);
//	ug_mutex_unlock (&uaria2->mutex);

	result = (int)ShellExecuteW (NULL, L"open", path_utf16, args_utf16,
	                             NULL, SW_HIDE);
	ug_free (args_utf16);
	ug_free (path_utf16);

	if (result > 32) {
		uaria2->launched = TRUE;
		return TRUE;
	}
	else {
		uaria2->error = UGET_ARIA2_ERROR_LAUNCH;
		return FALSE;
	}
}

#else

int  uget_aria2_launch (UgetAria2* uaria2)
{
	char** argv;
	int    temp;
	int    execpipe[2];
	pid_t  pid;

	if (uaria2->launched == TRUE)
		return TRUE;
	if (uaria2->path == NULL || uaria2->args == NULL)
		return FALSE;

	argv = ug_argv_from_cmd (uaria2->args, NULL, 1);
	argv[0] = uaria2->path;
	pipe (execpipe);
	// close on exec
	fcntl (execpipe[1], F_SETFD, fcntl (execpipe[1], F_GETFD) | FD_CLOEXEC);

	pid = fork();
	if (pid == -1) {
		// fork failed
		temp = -1;
	}
	else if (pid == 0) {
		// child process, [0] input, [1] output
		close (execpipe[0]);
		// on success, never returns
		execvp (uaria2->path, argv);
		temp = errno;
		write (execpipe[1], &temp, sizeof (temp));
		// doesn't matter what you exit with
		exit(0);
	}
	else {
		// parent process, [0] input, [1] output
		close (execpipe[1]);
		// if exec failed, read the child's errno value
		if (read (execpipe[0], &temp, sizeof(temp)) == sizeof(temp))
			temp = -1;
	}

	ug_argv_free (argv);
	// parent process
	if (temp == -1)
		uaria2->launched = FALSE;
	else
		uaria2->launched = TRUE;
	return uaria2->launched;
}

#endif  // _WIN32 || _WIN64

void uget_aria2_shutdown (UgetAria2* uaria2)
{
	UgJsonrpcObject*  jreq;
	UgJsonrpcObject*  jres;

	jreq = uget_aria2_alloc (uaria2, TRUE, TRUE);
	jreq->method_static = "aria2.shutdown";
	uget_aria2_request (uaria2, jreq);
	jres = uget_aria2_respond (uaria2, jreq);
	uget_aria2_recycle (uaria2, jreq);
	uget_aria2_recycle (uaria2, jres);
}

UgJsonrpcObject*  uget_aria2_alloc (UgetAria2* uaria2, int is_request, int has_response)
{
	UgJsonrpcObject* result;
	UgValue*         rpc_token = NULL;

	ug_mutex_lock (&uaria2->mutex);
	if (uaria2->recycled.length == 0)
		result = ug_jsonrpc_object_new ();
	else
		result = uaria2->recycled.at[--uaria2->recycled.length];
	ug_mutex_unlock (&uaria2->mutex);

	// RPC authorization secret token (aria2 v1.18.4)
	if (is_request) {
		ug_mutex_lock (&uaria2->mutex);
		if (uaria2->token) {
			ug_value_init_array (&result->params, 4);
			rpc_token = ug_value_alloc (&result->params, 1);
			rpc_token->type = UG_VALUE_STRING;
			rpc_token->c.string = ug_malloc (strlen (uaria2->token) + 7); // "token:"
			rpc_token->c.string[0] = 0;
			strcat (rpc_token->c.string, "token:");
			strcat (rpc_token->c.string, uaria2->token);
		}
		ug_mutex_unlock (&uaria2->mutex);
	}

	if (has_response)
		result->id.type = UG_VALUE_INT;
	return result;
}

void  uget_aria2_request (UgetAria2* uaria2, UgJsonrpcObject* request)
{
	ug_mutex_lock (&uaria2->mutex);
	*(UgJsonrpcObject**)ug_array_alloc (&uaria2->queuing, 1) = request;
	ug_mutex_unlock (&uaria2->mutex);
}

UgJsonrpcObject*  uget_aria2_respond (UgetAria2* uaria2, UgJsonrpcObject* request)
{
	UgJsonrpcObject* response = NULL;
	UgSLink*  prev_response;
	UgSLink*  prev;
	UgSLink*  link = NULL;
	int       completed_changed = 0;

	while (link == NULL) {
		while (completed_changed == uaria2->completed_changed) {
			// default: sleep 0.5 second
			ug_sleep (uaria2->polling_interval);
			continue;
		}

		ug_mutex_lock (&uaria2->completed_mutex);
		link = ug_slinks_find (&uaria2->requested, request, &prev);
		if (link) {
			// remove request
			if (prev)
				prev_response = uaria2->responsed.at + (prev - uaria2->requested.at);
			else
				prev_response = NULL;
			// get response & remove it
			response = (UgJsonrpcObject*)
					uaria2->responsed.at[link - uaria2->requested.at].data;
			ug_slinks_remove (&uaria2->requested, request,  prev);
			ug_slinks_remove (&uaria2->responsed, response, prev_response);
		}
		completed_changed = uaria2->completed_changed;
		ug_mutex_unlock (&uaria2->completed_mutex);
	}

	return response;
}

void  uget_aria2_recycle (UgetAria2* uaria2, UgJsonrpcObject* jobject)
{
	if (jobject) {
		ug_jsonrpc_object_clear (jobject);
		ug_mutex_lock (&uaria2->mutex);
		*(UgJsonrpcObject**)ug_array_alloc (&uaria2->recycled, 1) = jobject;
		ug_mutex_unlock (&uaria2->mutex);
	}
}

UgValue*  uget_aria2_clear_token (UgJsonrpcObject* jobject)
{
	UgValue*  rpc_token;

	if (jobject->params.type == UG_VALUE_ARRAY) {
		rpc_token = jobject->params.c.array->at;
		if (rpc_token->type == UG_VALUE_STRING &&
			rpc_token->c.string != NULL &&
			strncmp (rpc_token->c.string, "token:", 6) == 0)
		{
			ug_free (rpc_token->c.string);
			rpc_token->c.string = NULL;
			rpc_token->type = UG_VALUE_NONE;
			return rpc_token + 1;
		}
		return jobject->params.c.array->at;
	}

	return NULL;
}
