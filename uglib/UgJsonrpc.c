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

#ifdef DEBUG
#include <stdio.h>
#endif // DEBUG

#include <string.h>
#include <stddef.h>
#include <UgJsonrpc.h>

// ----------------------------------------------------------------------------
// UgJsonrpcError: JSON-RPC error object

const UgEntry  UgJsonrpcErrorEntry[] =
{
	{"code",    offsetof(UgJsonrpcError, code),    UG_ENTRY_INT,
			NULL, NULL},
	{"message", offsetof(UgJsonrpcError, message), UG_ENTRY_STRING,
			NULL, NULL},
	{"data",    offsetof(UgJsonrpcError, data),    UG_ENTRY_CUSTOM,
			ug_json_parse_value, ug_json_write_value},
	{NULL},
};

void  ug_jsonrpc_error_init(UgJsonrpcError* error)
{
	memset(error, 0, sizeof(UgJsonrpcError));
}

void  ug_jsonrpc_error_clear(UgJsonrpcError* error)
{
	// error
	ug_free(error->message);
	error->message = NULL;
	error->code = 0;
	ug_value_clear(&error->data);
}

// ----------------------------------------------------------------------------
// UgJsonrpcObject

const UgEntry  UgJsonrpcObjectEntry[] =
{
//	{"jsonrpc",  offsetof(UgJsonrpcObject, jsonrpc), UG_ENTRY_STRING,
//			NULL, NULL},
	{"id",       offsetof(UgJsonrpcObject, id),      UG_ENTRY_CUSTOM,
			ug_json_parse_value, ug_json_write_value},
	{"method",   offsetof(UgJsonrpcObject, method),  UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"params",   offsetof(UgJsonrpcObject, params),  UG_ENTRY_CUSTOM,
			ug_json_parse_value, ug_json_write_value},
	{"result",   offsetof(UgJsonrpcObject, result),  UG_ENTRY_CUSTOM,
			ug_json_parse_value, ug_json_write_value},
	{"error",    offsetof(UgJsonrpcObject, error),   UG_ENTRY_OBJECT,
			(void*)UgJsonrpcErrorEntry, NULL},
	{NULL},
};

UgJsonrpcObject* ug_jsonrpc_object_new(void)
{
	UgJsonrpcObject*  jobj;

	jobj = ug_malloc0(sizeof(UgJsonrpcObject));
	return jobj;
}

void  ug_jsonrpc_object_free(UgJsonrpcObject* jobj)
{
	// check NULL for uget_aria2_unref()
	if (jobj) {
		ug_jsonrpc_object_clear(jobj);
		ug_free(jobj);
	}
}

void  ug_jsonrpc_object_init(UgJsonrpcObject* jobj)
{
	memset(jobj, 0, sizeof(UgJsonrpcObject));
}

void  ug_jsonrpc_object_clear(UgJsonrpcObject* jobj)
{
	if (jobj == NULL)
		return;
	// jsonrpc
//	ug_free(jobj->jsonrpc);
//	jobj->jsonrpc = NULL;
	// id
	ug_value_clear(&jobj->id);
	// method
	ug_free(jobj->method);
	jobj->method = NULL;
	jobj->method_static = NULL;
	// params
	ug_value_clear(&jobj->params);
	// result
	ug_value_clear(&jobj->result);
	// error
	ug_jsonrpc_error_clear(&jobj->error);
}

void  ug_jsonrpc_object_clear_request(UgJsonrpcObject* jobj)
{
	ug_free(jobj->method);
	jobj->method = NULL;
	jobj->method_static = NULL;
	ug_value_clear(&jobj->params);
}

void  ug_jsonrpc_object_clear_response(UgJsonrpcObject* jobj)
{
	ug_value_clear(&jobj->result);
	ug_jsonrpc_error_clear(&jobj->error);
}

void  ug_json_write_rpc_object(UgJson* json, UgJsonrpcObject* jobj)
{
	ug_json_write_object_head(json);

	// jsonrpc
	ug_json_write_string(json, "jsonrpc");
	ug_json_write_string(json, "2.0");
//	ug_json_write_string(json, (jobj->jsonrpc) ? jobj->jsonrpc : "2.0");
	// id
	if (jobj->id.type != UG_VALUE_NONE) {
		if (jobj->id.name == NULL)
			ug_json_write_string(json, "id");
		ug_json_write_value(json, &jobj->id);
	}

	if (jobj->result.type == UG_VALUE_NONE && jobj->error.code == 0) {
		if (jobj->method || jobj->method_static) {
			ug_json_write_string(json, "method");
			ug_json_write_string(json,
					(jobj->method_static)? jobj->method_static : jobj->method);
			if (jobj->params.type != UG_VALUE_NONE) {
				if (jobj->params.name == NULL)
					ug_json_write_string(json, "params");
				ug_json_write_value(json, &jobj->params);
			}
		}
	}
	else if (jobj->result.type != UG_VALUE_NONE) {
		if (jobj->result.name == NULL)
			ug_json_write_string(json, "result");
		ug_json_write_value(json, &jobj->result);
	}
	else {
		ug_json_write_string(json, "error");
		ug_json_write_object_head(json);
		ug_json_write_entry(json, &jobj->error, UgJsonrpcErrorEntry);
		ug_json_write_object_tail(json);
	}

	ug_json_write_object_tail(json);
}

// ----------------------------------------------------------------------------
// UgJsonrpcArray: a UgJsonrpcObject array

//void  ug_jsonrpc_array_init(UgJsonrpcArray* joarray, int allocated_len);

void  ug_jsonrpc_array_clear(UgJsonrpcArray* joarray, int free_objects)
{
	if (free_objects) {
		ug_array_foreach_ptr(joarray,
				(UgForeachFunc) ug_jsonrpc_object_clear, NULL);
	}
	ug_array_clear(joarray);
}

UgJsonrpcObject*  ug_jsonrpc_array_find(UgJsonrpcArray* joarray, UgValue* id, int* index)
{
	UgJsonrpcObject** cur;
	UgJsonrpcObject** end;
	UgJsonrpcObject*  result = NULL;

	cur = joarray->at;
	end = cur + joarray->length;
	if (id->type == UG_VALUE_INT) {
		for (;  cur < end;  cur++) {
			// "cur[0] != NULL" for uget_aria2_match_response()
			if (cur[0] != NULL &&
			    cur[0]->id.type == UG_VALUE_INT &&
			    cur[0]->id.c.integer == id->c.integer)
			{
				result = *cur;
				break;
			}
		}
	}
	else if (id->type == UG_VALUE_STRING) {
		for (;  cur < end;  cur++) {
			if (cur[0]->id.type == UG_VALUE_STRING &&
			    strcmp(cur[0]->id.c.string, id->c.string) == 0)
			{
				result = *cur;
				break;
			}
		}
	}

	if (result && index)
		index[0] = cur - joarray->at;
	return result;
}

UgJsonrpcObject*  ug_jsonrpc_array_alloc(UgJsonrpcArray* joarray)
{
	UgJsonrpcObject**  jobj;

	jobj = ug_array_alloc(joarray, 1);
	*jobj = ug_jsonrpc_object_new();
	return *jobj;
}

UgJsonError  ug_json_parse_rpc_array(UgJson* json,
                                     const char* name, const char* value,
                                     void* jrarray, void* none)
{
	UgJsonrpcArray*   array;
	UgJsonrpcObject*  object;

	array = (UgJsonrpcArray*) jrarray;
	if (json->type != UG_JSON_OBJECT) {
//		if (json->type == UG_JSON_ARRAY)
//			ug_json_push(json, ug_json_parse_unknown, NULL, NULL);
		return UG_JSON_ERROR_RPC_INVALID;
	}

	object = ug_jsonrpc_array_alloc(array);
	ug_json_push(json, ug_json_parse_entry,
	             object, (void*)UgJsonrpcObjectEntry);
	return UG_JSON_ERROR_NONE;
}

void  ug_json_write_rpc_array(UgJson* json, UgJsonrpcArray* objects,
                              int  noArrayIfPossible)
{
	UgJsonrpcObject**   cur;
	UgJsonrpcObject**   end;

	if (noArrayIfPossible == FALSE || objects->length > 1)
		ug_json_write_array_head(json);

	end = objects->at + objects->length;
	cur = objects->at;
	for (;  cur < end;  cur++)
		ug_json_write_rpc_object(json, *cur);

	if (noArrayIfPossible == FALSE || objects->length > 1)
		ug_json_write_array_tail(json);
}

// ----------------------------------------------------------------------------
// UgJsonrpc: JSON-RPC

void  ug_jsonrpc_init(UgJsonrpc* jrpc, UgJson* json, UgBuffer* buffer)
{
	jrpc->json = json;
	jrpc->buffer = buffer;
	jrpc->data.id.previous = 0;
	jrpc->data.id.current = 0;
}

void  ug_jsonrpc_clear(UgJsonrpc* jrpc)
{
}

// ------------------------------------
// client API : set response == NULL if this is notify request

int  ug_jsonrpc_call(UgJsonrpc* jrpc,
                     UgJsonrpcObject* request,
                     UgJsonrpcObject* response)
{
	int    n;

//	if (jrpc->send.func == NULL || jrpc->receive.func == NULL)
//		return -1;

	jrpc->error = 0;
	// write --- start ---
	ug_json_begin_write(jrpc->json, 0, jrpc->buffer);
	// notify does NOT have id
	if (request->id.type != UG_VALUE_NONE)
		ug_value_clear(&request->id);
	if (response) {
		request->id.type = UG_VALUE_INT;
		request->id.c.integer = jrpc->data.id.current++;
	}
	ug_json_write_rpc_object(jrpc->json, request);
	ug_json_end_write(jrpc->json);
	// write --- end ---

#ifdef DEBUG
	printf("\n%.*s\n", jrpc->buffer->cur - jrpc->buffer->beg, jrpc->buffer->beg);
#endif // DEBUG

	// parser --- start ---
	ug_json_begin_parse(jrpc->json);
	if (response == NULL) {
		ug_json_push(jrpc->json, ug_json_parse_unknown,
		             NULL, NULL);
	}
	else {
		ug_json_push(jrpc->json, ug_json_parse_entry,
		             response, (void*)UgJsonrpcObjectEntry);
		ug_json_push(jrpc->json, ug_json_parse_object, NULL, NULL);
	}
	// send request
	n = jrpc->send.func(jrpc->send.data);
	if (n == -1)
		return -1;

	// notify does NOT have id.
	// If no new id assigned, you will not get any response.
	if (jrpc->data.id.previous == jrpc->data.id.current)
		return 0;
	jrpc->data.id.previous = jrpc->data.id.current;

	n = jrpc->receive.func(jrpc->receive.data);
	if (n == -1)
		return -1;
	n = ug_json_end_parse(jrpc->json);
	if (n < 0 || jrpc->error == 0)
		jrpc->error = n;
	// parser --- end ---

	return 0;   // no error
}

int  ug_jsonrpc_call_batch(UgJsonrpc* jrpc,
                           UgJsonrpcArray* request,
                           UgJsonrpcArray* response)
{
	UgJsonrpcObject** cur;
	UgJsonrpcObject** end;
	int        n;

//	if (jrpc->send.func == NULL || jrpc->receive.func == NULL)
//		return -1;

	jrpc->error = 0;
	if (request->length == 0)
		return 0;
	// write --- start ---
	ug_json_begin_write(jrpc->json, 0, jrpc->buffer);
	ug_json_write_array_head(jrpc->json);
	cur = request->at;
	end = request->at + request->length;
	for (;  cur < end;  cur++) {
		if (cur[0] == NULL)
			continue;
		// notify does NOT have id
		if (cur[0]->id.type != UG_VALUE_NONE) {
			ug_value_clear(&cur[0]->id);
			cur[0]->id.type = UG_VALUE_INT;
			cur[0]->id.c.integer = jrpc->data.id.current++;
		}
		ug_json_write_rpc_object(jrpc->json, cur[0]);
	}
	ug_json_write_array_tail(jrpc->json);
	ug_json_end_write(jrpc->json);
	// write --- end ---

#ifdef DEBUG
	printf("\n%.*s\n", jrpc->buffer->cur - jrpc->buffer->beg, jrpc->buffer->beg);
#endif

	// parser --- start ---
	ug_json_begin_parse(jrpc->json);
	if (response == NULL)
		ug_json_push(jrpc->json, ug_json_parse_unknown, NULL, NULL);
	else {
		ug_json_push(jrpc->json, ug_json_parse_rpc_array, response, NULL);
		ug_json_push(jrpc->json, ug_json_parse_array, NULL, NULL);
	}

	// send request
	n = jrpc->send.func(jrpc->send.data);
	if (n == -1)
		return -1;

	// notify does NOT have id.
	// If no new id assigned, you will not get any response.
	if (jrpc->data.id.previous == jrpc->data.id.current)
		return 0;
	jrpc->data.id.previous = jrpc->data.id.current;

	n = jrpc->receive.func(jrpc->receive.data);
	if (n == -1)
		return -1;
	n = ug_json_end_parse(jrpc->json);
	if (n < 0 || jrpc->error == 0)
		jrpc->error = n;
	// parser --- end ---

        return 0;   // no error
}

void ug_jsonrpc_array_free (UgJsonrpcArray* array)
{
        if (array == NULL)
                return;

        for (int index = 0; index < array->length; ++index) {
                UgJsonrpcObject* object = array->at[index];
                if (object)
                        ug_jsonrpc_object_free (object);
        }

        ug_array_clear (array);
        ug_free (array);
}

// ------------------------------------
// server API

int  ug_jsonrpc_receive(UgJsonrpc* jrpc,
                        UgJsonrpcObject* jr_object,
                        UgJsonrpcArray*  jr_array)
{
	int    n;
	int    type;
	UgJsonrpcObject*  jres;

	type = -1;
	jrpc->error = 0;
	jrpc->data.request.object = jr_object;
	jrpc->data.request.array  = jr_array;

	// parser --- start ---
	ug_json_begin_parse(jrpc->json);
	ug_json_push(jrpc->json, ug_json_parse_rpc_request,
	             jrpc, &type);
	// receive request
	n = jrpc->receive.func(jrpc->receive.data);
	if (n <= 0) {
		ug_json_end_parse(jrpc->json);
		return n;
	}
	n = ug_json_end_parse(jrpc->json);
	if (n < 0 || jrpc->error == 0)
		jrpc->error = n;
	// parser --- end ---

	if (jrpc->error == UG_JSON_ERROR_UNCOMPLETED) {
		// {"jsonrpc": "2.0", "error": {"code": -32700, "message": "Parse error"}, "id": null}
		jres = ug_jsonrpc_object_new();
		// "id": null
		jres->id.type = UG_VALUE_STRING;
		jres->id.c.string = NULL;
		// "error": {"code": -32700, "message": "Parse error"}
		jres->error.code = -32700;
		jres->error.message = "Parse error";
		// send response to client
		ug_jsonrpc_response(jrpc, jres);
		// clear response
		jres->error.message = NULL;
		ug_jsonrpc_object_free(jres);
		return -1;
	}
	if (type < UG_JSON_OBJECT) {
		// {"jsonrpc": "2.0", "error": {"code": -32600, "message": "Invalid Request"}, "id": null}
		jres = ug_jsonrpc_object_new();
		// "id": null
		jres->id.type = UG_VALUE_STRING;
		jres->id.c.string = NULL;
		// "error": {"code": -32600, "message": "Invalid Request"}
		jres->error.code = -32600;
		jres->error.message = "Invalid Request";
		// send response to client
		ug_jsonrpc_response(jrpc, jres);
		// clear response
		jres->error.message = NULL;
		ug_jsonrpc_object_free(jres);
		return -1;
	}

	return type;
}

int  ug_jsonrpc_response(UgJsonrpc* jrpc,
                         UgJsonrpcObject* response)
{
	int    n;

//	if (jrpc->send.func == NULL || jrpc->receive.func == NULL)
//		return -1;

	jrpc->error = 0;
	// write --- start ---
	ug_json_begin_write(jrpc->json, 0, jrpc->buffer);
	ug_json_write_rpc_object(jrpc->json, response);
	ug_json_end_write(jrpc->json);
	// write --- end ---

	// send responses
	n = jrpc->send.func(jrpc->send.data);
	if (n == -1)
		return -1;
	return n;
}

int  ug_jsonrpc_response_batch(UgJsonrpc* jrpc,
                               UgJsonrpcArray*  responses)
{
	UgJsonrpcObject** cur;
	UgJsonrpcObject** end;
	int        n;

//	if (jrpc->send.func == NULL || jrpc->receive.func == NULL)
//		return -1;

	jrpc->error = 0;
	if (responses->length == 0)
		return 0;
	// write --- start ---
	ug_json_begin_write(jrpc->json, 0, jrpc->buffer);
	ug_json_write_array_head(jrpc->json);
	cur = responses->at;
	end = responses->at + responses->length;
	for (n = 0;  cur < end;  cur++) {
		// don't output NULL object
		if (cur[0] == NULL)
			continue;
		// output object and increase count
		ug_json_write_rpc_object(jrpc->json, cur[0]);
		n++;
	}
	ug_json_write_array_tail(jrpc->json);
	ug_json_end_write(jrpc->json);
	// write --- end ---

	// if no object outputted, don't send response
	if (n == 0)
		return 0;

	// send responses
	n = jrpc->send.func(jrpc->send.data);
	if (n == -1)
		return -1;
	return n;
}

UgJsonError  ug_json_parse_rpc_request(UgJson* json,
                                       const char* name, const char* value,
                                       void* jsonrpc, void* type)
{
	UgJsonrpc* jrpc = jsonrpc;

	switch (json->type) {
	case UG_JSON_OBJECT:
		*(int*)type = UG_JSON_OBJECT;
		if (jrpc->data.request.object == NULL)
			break;
		ug_json_push(json, ug_json_parse_entry,
				jrpc->data.request.object, (void*)UgJsonrpcObjectEntry);
		return ug_json_parse_entry(json, name, value,
				jrpc->data.request.object, (void*)UgJsonrpcObjectEntry);

	case UG_JSON_ARRAY:
		*(int*)type = UG_JSON_ARRAY;
		if (jrpc->data.request.array == NULL)
			break;
		ug_json_push(json, ug_json_parse_rpc_array,
				jrpc->data.request.array, NULL);
		return ug_json_parse_rpc_array(json, name, value,
				jrpc->data.request.array, NULL);

	default:
		*(int*)type = json->type;
		return UG_JSON_ERROR_RPC_INVALID;
	}

	return UG_JSON_ERROR_NONE;
}

