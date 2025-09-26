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

#ifndef UG_JSONRPC_H
#define UG_JSONRPC_H

#include <UgArray.h>
#include <UgJson.h>
#include <UgValue.h>
#include <UgEntry.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UgJsonrpc             UgJsonrpc;
typedef struct UgJsonrpcError        UgJsonrpcError;
typedef struct UgJsonrpcObject       UgJsonrpcObject;
typedef UG_ARRAY(UgJsonrpcObject*)   UgJsonrpcArray;
typedef void (*UgJsonrpcServerFunc) (UgJsonrpc* jrpc, void* data, void* data2);

// UgEntry for JSON parser and writer
extern const UgEntry  UgJsonrpcErrorEntry[];
extern const UgEntry  UgJsonrpcObjectEntry[];

// ----------------------------------------------------------------------------
// UgJsonrpcError: JSON-RPC error object

struct UgJsonrpcError
{
//  JSON-RPC error code:
// -32700   Parse error.      Invalid JSON was received by the server.
//          An error occurred on the server while parsing the JSON text.
// -32600   Invalid Request   The JSON sent is not a valid Request object.
// -32601   Method not found  The method does not exist / is not available.
// -32602   Invalid params 	  Invalid method parameter(s).
// -32603   Internal error 	  Internal JSON-RPC error.
	int      code;
	char*    message;

	// This may be omitted.
	// A Primitive or Structured value that contains additional information about the error.
	// The value of this member is defined by the Server
	// (e.g. detailed error information, nested errors etc.).
	UgValue  data;
};

// ----------------------------------------------------------------------------
// UgJsonrpcObject

struct UgJsonrpcObject
{
//	char*        jsonrpc;   // version string

	// A Notification is a Request object without an "id" member.
	UgValue      id;

	// request
	char*        method;
	const char*  method_static;
	// This member MAY be omitted.
	UgValue      params;

	// response
	// This member MUST NOT exist if there was an error.
	UgValue         result;
	// This member MUST NOT exist if there was no error.
	UgJsonrpcError  error;
};

UgJsonrpcObject* ug_jsonrpc_object_new(void);
void  ug_jsonrpc_object_free(UgJsonrpcObject* jrobj);

void  ug_jsonrpc_object_init(UgJsonrpcObject* jrobj);
void  ug_jsonrpc_object_clear(UgJsonrpcObject* jrobj);

void  ug_jsonrpc_object_clear_request(UgJsonrpcObject* jrobj);
void  ug_jsonrpc_object_clear_response(UgJsonrpcObject* jrobj);

// check request, and return error code.
//int   ug_jsonrpc_object_check(UgJsonrpcObject* jrobj);

// parser: ug_json_parse_entry
// writer: ug_json_write_entry will output all value in UgJsonrpcObject.
// use below function to output JSON-RPC request and response object
void  ug_json_write_rpc_object(UgJson* json, UgJsonrpcObject* jrobj);

// ----------------------------------------------------------------------------
// UgJsonrpcArray: a UgJsonrpcObject array for batch

//void  ug_jsonrpc_array_init(UgJsonrpcArray* joarray, int allocated_len);
#define ug_jsonrpc_array_init(array, allocated_len)   \
                ug_array_init(array, sizeof(UgJsonrpcObject*), allocated_len)

// param free_objects: TRUE or FALSE
void  ug_jsonrpc_array_clear(UgJsonrpcArray* joarray, int free_objects);

// Free a batch returned by ug_jsonrpc_call_batch (frees elements + container).
void ug_jsonrpc_array_free (UgJsonrpcArray* array);

UgJsonrpcObject*  ug_jsonrpc_array_find(UgJsonrpcArray* joarray, UgValue* id, int* index);
UgJsonrpcObject*  ug_jsonrpc_array_alloc(UgJsonrpcArray* joarray);

// ------------------------------------
UgJsonError  ug_json_parse_rpc_array(UgJson* json,
                                     const char* name, const char* value,
                                     void* jrarray, void* none);
// param noArrayIfPossible: TRUE or FALSE
void         ug_json_write_rpc_array(UgJson* json, UgJsonrpcArray* objects,
                                     int  noArrayIfPossible);

// If program doesn't known incoming data type,
// this function can parse UgJsonrpcArray or UgJsonrpcObject.
// This function used by server.
UgJsonError  ug_json_parse_rpc_unknown(UgJson* json,
                                       const char* name, const char* value,
                                       void* jrarray, void* jrobject);
// ----------------------------------------------------------------------------
// UgJsonrpc: JSON-RPC Client & Server

//           +-----------------------------+
//           |        UgJsonrpcXXXX        |    UgJsonrpcArray
// buffer <--+--> UgJson <--> UgJsonrpc <--+-->       or
//           |                             |    UgJsonrpcObject
//           +-----------------------------+

// send/receive return input or output size
// send/receive return -1 = error
// receive      return  0 if connection close
typedef int  (*UgJsonrpcFunc) (void* target);

struct  UgJsonrpc
{
	UgJson*         json;
	UgBuffer*       buffer;
	UgJsonError     error;

	// input/output function
	struct {
		UgJsonrpcFunc  func;
		void*          data;
	} send, receive;

	union {
		// client
		struct {
			intptr_t  current;
			intptr_t  previous;
		} id;
		// server
		struct {
			UgJsonrpcObject*  object;
			UgJsonrpcArray*   array;
		} request;
	} data;
};

void  ug_jsonrpc_init(UgJsonrpc* jrpc, UgJson* json, UgBuffer* buffer);
void  ug_jsonrpc_clear(UgJsonrpc* jrpc);

// ------------------------------------
// client API : set response == NULL if this is notify request

// if error occurred, return -1
int  ug_jsonrpc_call(UgJsonrpc* jrpc,
                     UgJsonrpcObject* request,
                     UgJsonrpcObject* response);

int  ug_jsonrpc_call_batch(UgJsonrpc* jrpc,
                           UgJsonrpcArray* request,
                           UgJsonrpcArray* response);

// ------------------------------------
// server API

// return  0 if remote disconnected.
// return -1 if error occurred.
// if ok, return UG_JSON_ARRAY or UG_JSON_OBJECT and set jr_array or jr_object.
int  ug_jsonrpc_receive(UgJsonrpc* jrpc,
                        UgJsonrpcObject* jr_object,
                        UgJsonrpcArray*  jr_array);

int  ug_jsonrpc_response(UgJsonrpc* jrpc,
                         UgJsonrpcObject* jr_object);

int  ug_jsonrpc_response_batch(UgJsonrpc* jrpc,
                               UgJsonrpcArray* jr_array);

// This function used by UgJsonrpc server mode.
UgJsonError  ug_json_parse_rpc_request(UgJson* json,
                                       const char* name, const char* value,
                                       void* jsonrpc, void* type);

#ifdef __cplusplus
}
#endif

#endif  // UG_JSONRPC_H


