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

#include <stdio.h>
#include <UgSocket.h>
#include <UgJsonrpc.h>
#include <UgJsonrpcCurl.h>
#include <UgJsonrpcSocket.h>

#include <UgetAria2.h>
#include <curl/curl.h>

#if defined _WIN32 || defined _WIN64
#include <winsock2.h>
#else
#include <unistd.h>    // sleep()
#endif

#if defined _WIN32 || defined _WIN64
#define  ug_sleep                 Sleep
#else
#define  ug_sleep(millisecond)    usleep (millisecond * 1000)
#endif

static void socket_receiver (UgSocketServer* server,
                             SOCKET  client_fd, void* data)
{
	char  buf[5];

	ug_socket_server_ref (server);

//	ug_socket_recv (client_fd, buf, 5, 0);
	recv (client_fd, buf, 5, 0);
	puts ("socket server: get ping");
//	ug_socket_send (client_fd, "pong", 4, 0);
	send (client_fd, "pong", 4, 0);

//	ug_socket_close (client_fd);
	closesocket (client_fd);

	ug_socket_server_unref (server);
}

void test_socket_server_client (SOCKET server_fd, SOCKET client_fd)
{
	UgSocketServer*  server;
	char             buf[5];

	server = ug_socket_server_new (server_fd);
	ug_socket_server_set_receiver (server, socket_receiver, NULL);
	ug_socket_server_start (server);

	// --- client begin ---
//	ug_socket_send (client_fd, "ping", 4, 0);
	send (client_fd, "ping", 4, 0);
//	ug_socket_recv (client_fd, buf, 5, 0);
	recv (client_fd, buf, 5, 0);
	puts ("socket client: get pong");

//	ug_socket_close (client_fd);
//	close (client_fd);
	closesocket (client_fd);
	// --- client end ---

	ug_socket_server_stop (server);
	ug_socket_server_unref (server);
}

void test_socket (void)
{
	SOCKET server_fd;
	SOCKET client_fd;

	puts ("------------------");
//	server_fd = ug_socket_new (AF_INET, SOCK_STREAM, 0);
	server_fd = socket (AF_INET, SOCK_STREAM, 0);
	ug_socket_listen (server_fd, "127.0.0.1", "8088", 5);

//	client_fd = ug_socket_new (AF_INET, SOCK_STREAM, 0);
	client_fd = socket (AF_INET, SOCK_STREAM, 0);
	ug_socket_connect (client_fd, "127.0.0.1", "8088");

	test_socket_server_client (server_fd, client_fd);
	ug_sleep (1000);

#if !(defined _WIN32 || defined _WIN64)
	puts ("------------------");
//	server_fd = ug_socket_new (AF_INET, SOCK_STREAM, 0);
	server_fd = socket (AF_UNIX, SOCK_STREAM, 0);
	ug_socket_listen_unix (server_fd, "/tmp/test-socket-unix", -1, 5);

//	client_fd = ug_socket_new (AF_INET, SOCK_STREAM, 0);
	client_fd = socket (AF_UNIX, SOCK_STREAM, 0);
	ug_socket_connect_unix (client_fd, "/tmp/test-socket-unix", -1);

	test_socket_server_client (server_fd, client_fd);
	ug_sleep (1000);
#endif
}

// ----------------------------------------------------------------------------
// test parser & writer

void  parse_rpc_object (UgJsonrpcObject* jobj, const char* json_string)
{
	UgJson    json;
	int       code;

	ug_json_init (&json);
	ug_json_begin_parse (&json);
	ug_json_push (&json, ug_json_parse_entry, jobj, (void*)UgJsonrpcObjectEntry);
	ug_json_push (&json, ug_json_parse_object, NULL, NULL);
	code = ug_json_parse (&json, json_string, -1);
	printf ("ug_json_parse response %d\n", code);
	code = ug_json_end_parse (&json);
	printf ("ug_json_end_parse response %d\n", code);

	ug_json_final (&json);
}

static void
free_rpc_array (UgJsonrpcArray* array)
{
	int index;

	if (array == NULL)
		return;

	for (index = 0; index < array->length; index++)
		ug_jsonrpc_object_free (array->at[index]);

	ug_jsonrpc_array_clear (array, 0);
}

void  print_rpc_object (UgJsonrpcObject* jobj)
{
	UgJson    json;
	UgBuffer  buffer;

	ug_json_init (&json);
	ug_buffer_init (&buffer, 4096);
	ug_json_begin_write (&json, UG_JSON_FORMAT_INDENT, &buffer);
//	ug_json_write_entry (&json, req, requestEntry);
	ug_json_write_rpc_object (&json, jobj);
	ug_json_end_write (&json);
	ug_json_final (&json);

	ug_buffer_write_char (&buffer, '\0');
	puts (buffer.beg);
	ug_buffer_clear (&buffer, 1);
}

void  test_rpc_parser (void)
{
	UgJsonrpcArray*   jarray;
	UgJsonrpcObject*  jobj;
	const char*	json_string1 =
//		"{\"jsonrpc\": \"2.0\", \"method\": \"update\", \"params\": [1,2,3,4,5], \"id\":1}";
		"{\"jsonrpc\": \"2.0\", \"method\": \"foobar\", \"id\":2}";
	const char*	json_string2 =
		"{\"jsonrpc\": \"2.0\", \"error\": {\"code\": -32601, \"message\": \"Method not found.\"}, \"id\": 1}";
//		"{\"jsonrpc\": \"2.0\", \"result\": 19, \"id\":1}";
	const char*	json_string3 =
		"{\"id\":2,\"jsonrpc\":\"2.0\",\"result\":\"fc585c5fec7319bf\"}";

	puts ("----- test_rpc_parser()");

	jarray = malloc (sizeof (UgJsonrpcArray));
	ug_jsonrpc_array_init (jarray, 8);

	jobj = ug_jsonrpc_array_alloc (jarray);
	parse_rpc_object (jobj, json_string1);
	print_rpc_object (jobj);

	jobj = ug_jsonrpc_array_alloc (jarray);
	parse_rpc_object (jobj, json_string2);
	print_rpc_object (jobj);

	jobj = ug_jsonrpc_array_alloc (jarray);
	parse_rpc_object (jobj, json_string3);
	print_rpc_object (jobj);

	free_rpc_array (jarray);
	free (jarray);
}

void  test_rpc_curl_packet (UgJsonrpcCurl* jrcurl)
{
	int  error;
	UgJsonrpcObject*  request;
	UgJsonrpcObject*  response;
	const char* json_string =
			"{"
			  "\"jsonrpc\": \"2.0\","
			  "\"method\": \"aria2.addUri\","
			  "\"params\": [[\"http:\\/\\/localhost\\/file\"], {}],"
			  "\"id\":2"
			"}";

	request = ug_jsonrpc_object_new ();
	response = ug_jsonrpc_object_new ();
	parse_rpc_object (request, json_string);
	print_rpc_object (request);

	error = ug_jsonrpc_call (&jrcurl->rpc, request, response);
	if (error == 0)
		print_rpc_object (response);

	ug_jsonrpc_object_free (request);
	ug_jsonrpc_object_free (response);
}

void test_jsonrpc_curl (void)
{
//	UgJsonrpcHttp*   jrhttp;
	UgJsonrpcCurl*   jrcurl;
	UgJsonrpcArray*  jarray;

	puts ("----- test_jsonrpc_curl()");

//	jrhttp = malloc (sizeof (UgJsonrpcHttp));
//	ug_jsonrpc_http_init (jrhttp);

	jrcurl = malloc (sizeof (UgJsonrpcCurl));
	ug_jsonrpc_curl_init (jrcurl);

	jarray = malloc (sizeof (UgJsonrpcArray));
	ug_jsonrpc_array_init (jarray, 8);

//	ug_jsonrpc_http_set_client (jrhttp, "http://localhost:6800/jsonrpc");
//	test_rpc_http_packet (jrhttp);

	ug_jsonrpc_curl_set_url (jrcurl, "http://localhost:6800/jsonrpc");
//	test_rpc_curl_packet (jrcurl);

//	ug_jsonrpc_http_final (jrhttp);
//	free (jrhttp);

	ug_jsonrpc_curl_final (jrcurl);
	free (jrcurl);

	free_rpc_array (jarray);
	free (jarray);
}

// ----------------------------------------------------------------------------
// test UgJsonrpcSocket

static void jsonrpc_accepted (UgJsonrpc* jrpc, void* data, void* data2)
{
	UgJsonrpcObject* request;
	UgJsonrpcObject* response;
	int  result;

	request  = ug_jsonrpc_object_new ();
	response = ug_jsonrpc_object_new ();
	response->result.type = UG_VALUE_STRING;
	response->result.c.string = "pong";

	do {
		result = ug_jsonrpc_receive (jrpc, request, NULL);
		if (result == UG_JSON_OBJECT) {
			print_rpc_object (request);
			response->id.type = request->id.type;
			response->id.c    = request->id.c;
			ug_jsonrpc_response (jrpc, response);
			ug_jsonrpc_object_clear (request);
		}
	} while (result != 0);

	response->id.type = UG_JSON_NULL;
	response->result.c.string = NULL;
	ug_jsonrpc_object_free (response);
	ug_jsonrpc_object_free (request);
	puts ("server: client connection closed.");
}

void test_jsonrpc_socket (void)
{
	UgJsonrpcObject* request;
	UgJsonrpcObject* response;
	UgJsonrpcSocket* jclient;
	UgSocketServer*  server;
#if 0
	UgJsonrpcSocket* jserver;
#endif

	puts ("----- test_jsonrpc_socket()");

	server = ug_socket_server_new_addr ("127.0.0.1", "14777");
	if (server == NULL) {
		puts ("failed to create UgJsonrpcSocketServer");
		return;
	}

#if 0
	jserver = ug_malloc (sizeof (UgJsonrpcSocket));
	ug_jsonrpc_socket_init (jserver);
	ug_jsonrpc_socket_use_server (jserver, server, jsonrpc_accepted, NULL, NULL);
	ug_socket_server_start (server);
#else
	ug_socket_server_run_jsonrpc (server, jsonrpc_accepted, NULL, NULL);
#endif
	ug_sleep (1000);

	jclient = ug_malloc (sizeof (UgJsonrpcSocket));
	ug_jsonrpc_socket_init (jclient);
	ug_jsonrpc_socket_connect (jclient, "127.0.0.1", "14777");

	// ping server
	request = ug_jsonrpc_object_new ();
	response = ug_jsonrpc_object_new ();
	request->method_static = "ping";
	ug_jsonrpc_call (&jclient->rpc, request, response);
	print_rpc_object (response);
	ug_jsonrpc_object_free (response);
	ug_jsonrpc_object_free (request);

	ug_jsonrpc_socket_close (jclient);
	ug_jsonrpc_socket_final (jclient);
	ug_free (jclient);
	ug_sleep (1000);

#if 0
	ug_jsonrpc_socket_final(jserver);
#endif
	ug_socket_server_stop (server);
	ug_socket_server_unref (server);
}

// ----------------------------------------------------------------------------
// test_uget_aria2

void test_uget_aria2_rpc_object (UgetAria2* uaria2)
{
	UgValue*          value;
	UgValue*          vobj;
	UgJsonrpcObject*  req1;
	UgJsonrpcObject*  res1;
	UgJsonrpcObject*  req2;
	UgJsonrpcObject*  res2;

	// request 1
	req1 = uget_aria2_alloc (uaria2, TRUE, TRUE);
	req1->method_static = "aria2.getGlobalStat";
	uget_aria2_request (uaria2, req1);

	// request 2
	req2 = uget_aria2_alloc (uaria2, TRUE, TRUE);
	req2->method_static = "aria2.changeGlobalOption";
	ug_value_init_array (&req2->params, 2);
	vobj = ug_value_alloc (&req2->params, 1);
	ug_value_init_object (vobj, 2);
	value = ug_value_alloc (vobj, 1);
	value->name = "max-overall-download-limit";
	value->type = UG_VALUE_STRING;
	value->c.string = "100k";
	value = ug_value_alloc (vobj, 1);
	value->name = "max-overall-upload-limit";
	value->type = UG_VALUE_STRING;
	value->c.string = "100k";
	uget_aria2_request (uaria2, req2);

	// response 1
	res1 = uget_aria2_respond (uaria2, req1);
	if (res1) {
		print_rpc_object (res1);
		ug_value_sort_name (&res1->result);
		value = ug_value_find_name (&res1->result, "downloadSpeed");
		printf ("current downloadSpeed %d\n", ug_value_get_int (value));
	}
	// recycle 1
	uget_aria2_recycle (uaria2, req1);
	uget_aria2_recycle (uaria2, res1);

	// response 2
	res2 = uget_aria2_respond (uaria2, req2);
	if (res2) {
		print_rpc_object (res2);
	}
	// recycle 2
	ug_value_foreach (&req2->params, ug_value_set_name_string, NULL);
	uget_aria2_recycle (uaria2, req2);
	uget_aria2_recycle (uaria2, res2);
}

void test_uget_aria2 (void)
{
	UgetAria2*    uaria2;

	puts ("----- test_uget_aria2()");

	uaria2 = uget_aria2_new ();
	uget_aria2_start_thread (uaria2);

#if defined _WIN32 || defined _WIN64
	uget_aria2_set_path (uaria2, "C:\\Program Files\\uGet\\bin\\aria2c.exe");
#endif
	uget_aria2_launch (uaria2);
	uaria2->shutdown = TRUE;
	test_uget_aria2_rpc_object (uaria2);
//	uget_aria2_shutdown (uaria2);

	uget_aria2_stop_thread (uaria2);
	uget_aria2_unref (uaria2);
	ug_sleep (2000);
}

// ----------------------------------------------------------------------------
// main

int   main (void)
{
#if defined _WIN32 || defined _WIN64
	WSADATA WSAData;

	WSAStartup (MAKEWORD (2, 2), &WSAData);
#endif // _WIN32 || _WIN64
	// libcurl
	curl_global_init (CURL_GLOBAL_ALL);

	test_socket ();
	test_rpc_parser ();
	test_jsonrpc_socket ();
	test_jsonrpc_curl ();
	test_uget_aria2 ();

	// libcurl
	curl_global_cleanup ();

#if defined _WIN32 || defined _WIN64
	WSACleanup ();
#endif

	return 0;
}

