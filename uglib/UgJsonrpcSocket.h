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

#ifndef UG_JSONRPC_SOCKET_H
#define UG_JSONRPC_SOCKET_H

#include <UgSocket.h>    // SOCKET, INVALID_SOCKET
#include <UgJsonrpc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UgJsonrpcSocket   UgJsonrpcSocket;
typedef struct UgSocketServer    UgSocketServer;

// ----------------------------------------------------------------------------
// UgJsonrpcSocket: JSON-RPC over Socket

//           +-----------------------------+
//           |       UgJsonrpcSocket       |    UgJsonrpcArray
// buffer <--+--> UgJson <--> UgJsonrpc <--+-->       or
//           |                             |    UgJsonrpcObject
//           +-----------------------------+

// 'socket' is a SOCKET, not an int: on 64-bit Windows SOCKET is a 64-bit
// UINT_PTR and an int would truncate the handle (and mangle INVALID_SOCKET,
// which is (SOCKET)~0). On Unix UgSocket.h defines SOCKET as int.
#define UG_JSONRPC_SOCKET_MEMBERS  \
	UgJson           json;         \
	UgJsonrpc        rpc;          \
	UgBuffer         buffer;       \
	SOCKET           socket

struct  UgJsonrpcSocket
{
	UG_JSONRPC_SOCKET_MEMBERS;
/*	// ------ UgJsonrpcSocket members ------
	UgJson           json;
	UgJsonrpc        rpc;
	UgBuffer         buffer;
	SOCKET           socket;
 */
};

void  ug_jsonrpc_socket_init (UgJsonrpcSocket* jrsock);
void  ug_jsonrpc_socket_final (UgJsonrpcSocket* jrsock);

// ------------------------------------
// Client API
void  ug_jsonrpc_socket_close (UgJsonrpcSocket* jrsock);
int   ug_jsonrpc_socket_connect (UgJsonrpcSocket* jrsock,
                                 const char* addr,
                                 const char* port_or_serv);

#if !(defined _WIN32 || defined _WIN64)
int   ug_jsonrpc_socket_connect_unix (UgJsonrpcSocket* jrsock,
                                      const char* path, int path_len);
#endif // ! (_WIN32 || _WIN64)

int   ug_jsonrpc_socket_send (UgJsonrpcSocket* jrsock);
int   ug_jsonrpc_socket_receive (UgJsonrpcSocket* jrsock);

// ------------------------------------
// Server API

// one thread handle all connection
void  ug_jsonrpc_socket_use_server (UgJsonrpcSocket* jrsock,
                                    UgSocketServer* server,
                                    UgJsonrpcServerFunc callback,
                                    void* data, void* data2);

// one thread handle one connection
void  ug_socket_server_run_jsonrpc (UgSocketServer* server,
                                    UgJsonrpcServerFunc callback,
                                    void* data, void* data2);

#ifdef __cplusplus
}
#endif

#endif  // UG_JSONRPC_SOCKET_H

