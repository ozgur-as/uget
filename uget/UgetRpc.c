/*
 *
 *   Copyright (C) 2015-2020 by C.H. Huang
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

#include <errno.h>
#include <stdlib.h>   // srand()
#include <time.h>     // time()

#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#endif

#include <UgUtil.h>
#include <UgString.h>
#include <UgSocket.h>
#include <UgFileUtil.h>
#include <UgetRpc.h>
#include <UgStdio.h>

#define UGET_RPC_PORT      "14777"
#define UGET_RPC_ADDR      "127.0.0.1"
//#define UGET_RPC_NAME      "RPC-socket"
//#define UGET_RPC_NAME_ABS  "com.ugetdm.uget"
//#define UGET_RPC_LIMIT     50

static void uget_rpc_on_destroy (void* data);
static void on_accepted (UgJsonrpc* jrpc, UgetRpc* urpc, void* data);
static void set_invalid_request (UgJsonrpcObject* jobj);
static void backup_data_file (UgetOptionValue* uoval, const char* dir);

UgetRpc*  uget_rpc_new (const char* backup_dir)
{
	UgetRpc*  urpc;

#if defined _WIN32 || defined _WIN64
	WSADATA  WSAData;
	WSAStartup (MAKEWORD (2, 2), &WSAData);
#endif

	urpc = ug_malloc (sizeof (UgetRpc));
	ug_jsonrpc_socket_init ((UgJsonrpcSocket*) urpc);

	urpc->server = NULL;
	ug_jsonrpc_object_init (&urpc->jobject);
	ug_jsonrpc_array_init (&urpc->jarray, 8);

	ug_option_init (&urpc->option);
	ug_list_init (&urpc->queue);
	ug_mutex_init (&urpc->queue_lock);
	if (backup_dir)
		urpc->backup_dir = ug_strdup (backup_dir);
	else
		urpc->backup_dir = NULL;
#ifdef USE_UNIX_DOMAIN_SOCKET
	urpc->socket_path = NULL;
	urpc->socket_path_len = 0;
#endif
	return urpc;
}

void  uget_rpc_free (UgetRpc* urpc)
{
	if (urpc->server == NULL)
		uget_rpc_on_destroy (urpc);
	else {
		ug_socket_server_stop (urpc->server);
		ug_socket_server_unref (urpc->server);
	}
}

#ifdef USE_UNIX_DOMAIN_SOCKET
void  uget_rpc_use_unix_socket (UgetRpc* urpc, const char* path, int path_len)
{
	if (path_len > 0 && path[0] == 0) {
		// for Linux abstract socket
		urpc->socket_path = ug_malloc (path_len);
		memcpy (urpc->socket_path, path, path_len);
	}
	else {
		// for unix domain socket
		if (path_len == -1)
			path_len = strlen (path);
		urpc->socket_path = ug_strndup (path, path_len);
	}
	urpc->socket_path_len = path_len;
}
#endif  // USE_UNIX_DOMAIN_SOCKET

static void uget_rpc_on_destroy (void* data)
{
	UgetRpc* urpc = (UgetRpc*) data;

#if defined _WIN32 || defined _WIN64
	WSACleanup ();
#endif

	ug_jsonrpc_socket_final ((UgJsonrpcSocket*) urpc);
	ug_jsonrpc_object_clear (&urpc->jobject);
	ug_jsonrpc_array_clear (&urpc->jarray, TRUE);

	ug_option_final (&urpc->option);
	ug_list_foreach (&urpc->queue, (UgForeachFunc) uget_rpc_cmd_free, NULL);
//	ug_list_clear (&urpc->queue, FALSE);
	ug_mutex_clear (&urpc->queue_lock);

#ifdef USE_UNIX_DOMAIN_SOCKET
	// Don't delete file if path is abstract socket names (begin with 0)
	if (urpc->server && urpc->socket_path[0])
		ug_unlink (urpc->socket_path);
	ug_free (urpc->socket_path);
#endif
	ug_free (urpc->backup_dir);
	ug_free (urpc);
}

int  uget_rpc_do_request (UgetRpc* urpc, UgJsonrpcObject* jobj)
{
	UgValueArray*  array;
	UgetRpcReq*    req;
	UgetRpcCmd*    cmd;
	UgLink*        urilink;
	const char*    method;
	int            index;

	if (jobj->method_static)
		method = jobj->method_static;
	else
		method = jobj->method;

	if (method == NULL) {
		set_invalid_request (jobj);
		return FALSE;
	}

	if (strcmp (method, "uget.present") == 0) {
		req = uget_rpc_req_new ();
		req->method_id = UGET_RPC_PRESENT;
		// add to queue
		ug_mutex_lock (&urpc->queue_lock);
		ug_list_append (&urpc->queue, (UgLink*) req);
		ug_mutex_unlock (&urpc->queue_lock);
		// response OK
		// {"jsonrpc": "2.0", "result": true, "id": 1}
		ug_jsonrpc_object_clear_request (jobj);
		jobj->result.type = UG_VALUE_BOOL;
		jobj->result.c.boolean = TRUE;
		return TRUE;
	}
	else if (strcmp (method, "uget.sendCommand") == 0) {
		if (jobj->params.type != UG_VALUE_ARRAY) {
			set_invalid_request (jobj);
			return FALSE;
		}
		// parse params
		cmd = uget_rpc_cmd_new ();
		ug_option_clear (&urpc->option);
		ug_option_set_parser (&urpc->option, ug_option_parse_entry,
		                      &cmd->value, uget_option_entry);
		array = jobj->params.c.array;
		for (index = 0;  index < array->length;  index++)
			ug_option_parse (&urpc->option, array->at[index].c.string, -1);
		// get URIs from command-line
		for (index = 0;  index < urpc->option.others.length;  index++) {
			urilink = ug_link_new();
			urilink->data = urpc->option.others.at[index];
			urpc->option.others.at[index] = NULL;
			ug_list_append (&cmd->uris, urilink);
		}
		// get URIs from text file
		if (cmd->value.input_file)
			ug_file_get_lines (cmd->value.input_file, &cmd->uris);
		// check arguments
		if (cmd->uris.size == 0 && uget_option_value_has_ctrl (&cmd->value)) {
			set_invalid_request (jobj);
			if (cmd->uris.size == 0) {
				jobj->error.data.type = UG_VALUE_STRING;
				jobj->error.data.c.string = ug_strdup ("No URIs");
			}
			uget_rpc_cmd_free (cmd);
			return FALSE;
		}
		// backup cookie and post file
		if (urpc->backup_dir)
			backup_data_file (&cmd->value, urpc->backup_dir);
		// add to queue
		ug_mutex_lock (&urpc->queue_lock);
		ug_list_append (&urpc->queue, (UgLink*) cmd);
		ug_mutex_unlock (&urpc->queue_lock);
		// response OK
		// {"jsonrpc": "2.0", "result": true, "id": 1}
		ug_jsonrpc_object_clear_request (jobj);
		jobj->result.type = UG_VALUE_BOOL;
		jobj->result.c.boolean = TRUE;
		return TRUE;
	}

	// {"jsonrpc": "2.0", "error": {"code": -32601, "message": "Method not found"}, "id": "1"}
	ug_jsonrpc_object_clear_request (jobj);
	ug_free (jobj->error.message);
	jobj->error.code = -32601;
	jobj->error.message = ug_strdup ("Method not found");
	return FALSE;
}

void uget_rpc_send_command (UgetRpc* urpc, int argc, char** argv)
{
	UgJsonrpcObject*  jobj;
	UgJsonrpcObject*  jres;
	UgValue*  value;
	int       index;

	if (argc == 0) {
		if (urpc->server == NULL)
			uget_rpc_present (urpc);
		return;
	}

	jobj = ug_jsonrpc_object_new ();
	jobj->method_static = "uget.sendCommand";
	ug_value_init_array (&jobj->params, argc);
	value = ug_value_alloc (&jobj->params, argc);
	for (index = 0;  index < argc;  index++, value++) {
//		value->name = NULL;
		value->type = UG_VALUE_STRING;
#if (defined _WIN32 || defined _WIN64) && defined HAVE_GLIB
		if (argv[index] && g_utf8_validate (argv[index], -1, NULL) == FALSE) {
			value->c.string = g_locale_to_utf8 (argv[index], -1,
			                                    NULL, NULL, NULL);
		} else
#endif // (_WIN32 || _WIN64) && HAVE_GLIB
		value->c.string = ug_strdup (argv[index]);

		// replace invalid characters \/:*?"<>| by _ in filename.
		if (strncmp (value->c.string, "--filename=", 11) == 0)
			ug_str_replace_chars (value->c.string +11, "\\/:*?\"<>|", '_');
	}

	if (urpc->server)
		uget_rpc_do_request (urpc, jobj);
	else {
		jres = ug_jsonrpc_object_new ();
#ifdef USE_UNIX_DOMAIN_SOCKET
		if (urpc->socket_path) {
			ug_jsonrpc_socket_connect_unix ((UgJsonrpcSocket*) urpc,
			                                urpc->socket_path,
			                                urpc->socket_path_len);
		}
		else
#endif
		ug_jsonrpc_socket_connect ((UgJsonrpcSocket*) urpc,
		                           UGET_RPC_ADDR, UGET_RPC_PORT);
		ug_jsonrpc_call (&urpc->rpc, jobj, jres);
		ug_jsonrpc_object_free (jres);
	}
	ug_jsonrpc_object_free (jobj);
}

void  uget_rpc_present (UgetRpc* urpc)
{
	UgJsonrpcObject*  jobj;
	UgJsonrpcObject*  jres;

	jobj = ug_jsonrpc_object_new ();
	jobj->method_static = "uget.present";

	if (urpc->server)
		uget_rpc_do_request (urpc, jobj);
	else {
		jres = ug_jsonrpc_object_new ();
#ifdef USE_UNIX_DOMAIN_SOCKET
		if (urpc->socket_path) {
			ug_jsonrpc_socket_connect_unix ((UgJsonrpcSocket*) urpc,
			                                urpc->socket_path,
			                                urpc->socket_path_len);
		}
		else
#endif
		ug_jsonrpc_socket_connect ((UgJsonrpcSocket*) urpc,
		                           UGET_RPC_ADDR, UGET_RPC_PORT);
		ug_jsonrpc_call (&urpc->rpc, jobj, jres);
		ug_jsonrpc_object_free (jres);
	}
	ug_jsonrpc_object_free (jobj);
}

int   uget_rpc_start_server (UgetRpc* urpc, int detect_server)
{
	SOCKET  fd;
	int     result;
	int     in_progress = FALSE;
	int     opt_value;
	socklen_t     opt_length;
	fd_set  fdset;
	struct timeval timeout;

	if (urpc->server) {
		ug_socket_server_start (urpc->server);
		return TRUE;
	}

	// detect server
	if (detect_server) {
#ifdef USE_UNIX_DOMAIN_SOCKET
		if (urpc->socket_path) {
			fd = socket (AF_UNIX, SOCK_STREAM, 0);
			ug_socket_set_blocking (fd, FALSE);
			result = ug_socket_connect_unix (fd,
			                                 urpc->socket_path,
			                                 urpc->socket_path_len);
			if (errno == EINPROGRESS)
				in_progress = TRUE;
		}
		else
#endif  // USE_UNIX_DOMAIN_SOCKET
		{
			fd = socket (AF_INET, SOCK_STREAM, 0);
			ug_socket_set_blocking (fd, FALSE);
			result = ug_socket_connect (fd, UGET_RPC_ADDR, UGET_RPC_PORT);
#if (defined _WIN32 || defined _WIN64)
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				in_progress = TRUE;
#else
			if (errno == EINPROGRESS)
				in_progress = TRUE;
#endif
		}

		// connect with timeout (non-blocking)
		if (result < 0 && in_progress) {
			FD_ZERO (&fdset);
			FD_SET (fd, &fdset);
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
			// select() return  0 if time limit expired.
			// select() return -1 if error.
			if (select (fd+1, NULL, &fdset, NULL, &timeout) > 0) {
				opt_value = 1;
				opt_length = sizeof (opt_value);
				getsockopt (fd, SOL_SOCKET, SO_ERROR, (void*)(&opt_value), &opt_length);
				if (opt_value == 0)
					result = 0;    // connect OK
			}
		}

		closesocket (fd);
		if (result != -1) {
			return FALSE;
		}
	}

	// create server
#ifdef USE_UNIX_DOMAIN_SOCKET
	if (urpc->socket_path) {
		// Don't delete file if path is abstract socket names (begin with 0)
		if (urpc->socket_path[0])
			ug_unlink (urpc->socket_path);
		urpc->server = ug_socket_server_new_unix (urpc->socket_path,
		                                          urpc->socket_path_len);
	}
	else
#endif
	urpc->server = ug_socket_server_new_addr (UGET_RPC_ADDR, UGET_RPC_PORT);
	if (urpc->server == NULL)
		return FALSE;
	urpc->server->destroy.func = uget_rpc_on_destroy;
	urpc->server->destroy.data = urpc;
	ug_jsonrpc_socket_use_server ((UgJsonrpcSocket*) urpc, urpc->server,
	                              (UgJsonrpcServerFunc) on_accepted,
	                              urpc, NULL);
	ug_socket_server_start (urpc->server);
	return TRUE;
}

void  uget_rpc_stop_server  (UgetRpc* urpc)
{
	if (urpc->server)
		ug_socket_server_stop (urpc->server);
}

int   uget_rpc_has_request (UgetRpc* urpc)
{
	if (urpc->queue.head)
		return TRUE;
	else
		return FALSE;
}

UgetRpcReq* uget_rpc_get_request (UgetRpc* urpc)
{
	UgetRpcReq*  link;

	ug_mutex_lock (&urpc->queue_lock);
	link = (UgetRpcReq*) urpc->queue.head;
	if (link)
		ug_list_remove (&urpc->queue, (UgLink*) link);
	ug_mutex_unlock (&urpc->queue_lock);
	return link;
}

static void on_accepted (UgJsonrpc* jrpc, UgetRpc* urpc, void* data)
{
	UgJsonrpcObject*  jobj;
	UgJsonType  type;
	int         index;

	type = ug_jsonrpc_receive (jrpc, &urpc->jobject, &urpc->jarray);
	if (type == UG_JSON_OBJECT) {
		uget_rpc_do_request (urpc, &urpc->jobject);
		if (urpc->jobject.id.type != UG_VALUE_NONE)
			ug_jsonrpc_response (jrpc, &urpc->jobject);
		ug_jsonrpc_object_clear (&urpc->jobject);
	}
	else if (type == UG_JSON_ARRAY) {
		for (index = 0;  index < urpc->jarray.length;  index++) {
			jobj = urpc->jarray.at[index];
			uget_rpc_do_request (urpc, jobj);
			if (jobj->id.type == UG_VALUE_NONE && jobj->error.code == 0) {
				ug_jsonrpc_object_free (jobj);
				urpc->jarray.at[index] = NULL;
			}
		}
		ug_jsonrpc_response_batch (jrpc, &urpc->jarray);
		ug_jsonrpc_array_clear (&urpc->jarray, TRUE);
	}
}

static void set_invalid_request (UgJsonrpcObject* jobj)
{
	// {"jsonrpc": "2.0", "error": {"code": -32600, "message": "Invalid Request"}, "id": null}
	ug_jsonrpc_object_clear_request (jobj);
	ug_free (jobj->error.message);
	jobj->error.code = -32600;
	jobj->error.message = ug_strdup ("Invalid Request");
}

static void backup_data_file (UgetOptionValue* uoval, const char* dir)
{
	int   num;
	char* fpath;

	srand ((unsigned int) time(NULL));
	num = rand ();

	// backup cookie file
	if (uoval->http.cookie_file) {
		fpath = ug_strdup_printf ("%s" UG_DIR_SEPARATOR_S "%X.cookie",
		                          dir, num);
		if (ug_file_copy (uoval->http.cookie_file, fpath) == -1)
			ug_free (fpath);
		else {
			ug_free (uoval->http.cookie_file);
			uoval->http.cookie_file = fpath;
			fpath = NULL;
		}
	}

	// backup post file
	if (uoval->http.post_file) {
		fpath = ug_strdup_printf ("%s" UG_DIR_SEPARATOR_S "%X.post",
		                          dir, num);
		if (ug_file_copy (uoval->http.post_file, fpath) == -1)
			ug_free (fpath);
		else {
			ug_free (uoval->http.post_file);
			uoval->http.post_file = fpath;
			fpath = NULL;
		}
	}
}

// ----------------------------------------------------------------------------
// UgetRpcReq

UgetRpcReq*  uget_rpc_req_new (void)
{
	UgetRpcReq*  req;

	req = ug_malloc0 (sizeof (UgetRpcReq));
	req->free = ug_free;
	return req;
}

// ----------------------------------------------------------------------------
// UgetRpcCmd

UgetRpcCmd*  uget_rpc_cmd_new (void)
{
	UgetRpcCmd*  cmd;

	cmd = ug_malloc (sizeof (UgetRpcCmd));
	cmd->method_id = UGET_RPC_SEND_COMMAND;
	cmd->next = NULL;
	cmd->prev = NULL;
	cmd->free = (UgDeleteFunc) uget_rpc_cmd_free;

	uget_option_value_init (&cmd->value);
	ug_list_init (&cmd->uris);
	return cmd;
}

void  uget_rpc_cmd_free (UgetRpcCmd* urcmd)
{
	UgLink* urilink;

	uget_option_value_clear (&urcmd->value);
	for (urilink = urcmd->uris.head;  urilink;  urilink = urilink->next)
		ug_free (urilink->data);
	ug_list_clear (&urcmd->uris, TRUE);
	ug_free (urcmd);
}
