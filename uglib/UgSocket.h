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

#ifndef UG_SOCKET_H
#define UG_SOCKET_H

#include <stdint.h>
#include <UgThread.h>
#include <UgDefine.h>

#if defined _WIN32 || defined _WIN64
#if !defined _WIN32_WINNT || _WIN32_WINNT < 0x0501
#  define _WIN32_WINNT    0x0501   // WinXP
#endif
//#include <winsock2.h>
#include <Ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>     // socket api
#include <sys/un.h>         // struct sockaddr_un
#include <netinet/in.h>     // struct sockaddr_in
#include <unistd.h>         // close()
#define closesocket     close
#define SOCKET          int
#define INVALID_SOCKET  (-1)
#define SOCKET_ERROR	(-1)
#endif // _WIN32 || _WIN64

#ifdef __cplusplus
extern "C" {
#endif

// domain    AF_INET, AF_INET6, or AF_UNIX
// type      SOCK_STREAM or SOCK_DGRAM...etc
// protocol  IPPROTO_IP, IPPROTO_TCP, or IPPROTO_UDP
//SOCKET ug_socket_new (int domain, int type, int protocol);
#define ug_socket_new         socket

//void  ug_socket_close (SOCKET fd);
#define ug_socket_close       closesocket

//void  ug_socket_shutdown (SOCKET fd, int how);
#define ug_socket_shutdown    shutdown

// recv() return  0 if remote disconnected.
//int   ug_socket_send (SOCKET fd, char* buffer, int buffer_len, int flag);
//int   ug_socket_recv (SOCKET fd, char* buffer, int buffer_len, int flag);
#define ug_socket_send        send;
#define ug_socket_recv        recv;

// ------------------------------------
// Client API

// IPv4 or IPv6
// return SOCKET_ERROR(-1) if error occurred.
int  ug_socket_connect (SOCKET fd, const char* addr, const char* port_or_serv);

#if !(defined _WIN32 || defined _WIN64)
// UNIX Domain Socket
int  ug_socket_connect_unix (SOCKET fd, const char* path, int path_len);
#endif // ! (_WIN32 || _WIN64)

// return TRUE or FALSE
int  ug_socket_set_blocking (SOCKET fd, int is_blocking);

// ------------------------------------
// Server API

// IPv4 or IPv6
// backlog = incomplete connection queue + completed connection queue
//           use 5 in normal case
// return SOCKET, return SOCKET_ERROR in error
int  ug_socket_listen (SOCKET fd, const char* addr, const char* port_or_serv,
                       int backlog);

#if !(defined _WIN32 || defined _WIN64)
// UNIX Domain Socket
int  ug_socket_listen_unix (SOCKET fd, const char* path, int path_len, int backlog);
#endif // ! (_WIN32 || _WIN64)

/*
// ------------------------------------
// UgSocketServer

	// sample code:
	UgSocketServer*  server;
	SOCKET           server_fd;

	server_fd = socket (AF_INET, SOCK_STREAM, 0);
	ug_socket_listen (server_fd, "127.0.0.1", "80", 5);

	server = ug_socket_server_new (server_fd);
	ug_socket_server_set_receiver (server, callback, callback_data);
	ug_socket_server_start (server);

	// --- now client can connect to server ---

	ug_socket_server_stop (server);
	ug_socket_server_unref (server);
 */

typedef struct UgSocketServer       UgSocketServer;
typedef void (*UgSocketServerFunc) (UgSocketServer* server,
                                    SOCKET client_socket,
                                    void*  data);

struct UgSocketServer {
	UgThread  thread;
	int       ref_count;
	uint8_t   stopping;
	uint8_t   stopped;

	SOCKET    socket;
	fd_set    read_fds;

	// client address used by accept()
	socklen_t client_addr_len;
	union {
#if !(defined _WIN32 || defined _WIN64)
//		struct sockaddr_un  unix_path;
#endif
		struct sockaddr_in  inet;
		struct sockaddr_in6 inet6;
	} client_addr;

	// callback after accept()
	struct {
		UgSocketServerFunc  func;
		void*               data;
	} receiver;

	// destroy notification
	struct {
		UgNotifyFunc  func;
		void*         data;
	} destroy;

	// user data
	struct {
		void*  data;
		void*  data2;
		void*  data3;
	} user;
};

UgSocketServer* ug_socket_server_new (SOCKET server_fd);
UgSocketServer* ug_socket_server_new_addr (const char* addr,
                                           const char* port_or_serv);
#if !(defined _WIN32 || defined _WIN64)
// UNIX Domain Socket
UgSocketServer* ug_socket_server_new_unix (const char* path,
                                           int         path_len);
#endif

void  ug_socket_server_ref (UgSocketServer* server);
void  ug_socket_server_unref (UgSocketServer* server);

void  ug_socket_server_close (UgSocketServer* server);

// Program will call user specified function after accept()
void  ug_socket_server_set_receiver (UgSocketServer* server,
                                     UgSocketServerFunc func,
                                     void* data);

// start/stop thread for accept()
int   ug_socket_server_start (UgSocketServer* server);
void  ug_socket_server_stop (UgSocketServer* server);


#ifdef __cplusplus
}
#endif

#endif  // UG_SOCKET_H

