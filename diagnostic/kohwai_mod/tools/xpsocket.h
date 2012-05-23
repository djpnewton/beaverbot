#ifndef _XPSOCKET_H_
#define _XPSOCKET_H_

#ifdef WIN32

#include <winsock2.h>

#else // (not) WIN32

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#define SOCKET int
#define SOCKET_ERROR -1
#define INVALID_SOCKET 0
#define SD_BOTH 2

#endif // WIN32

typedef struct xpsocket_t* xpsocket_handle;
typedef void (*xpsocket_receive_callback)(xpsocket_handle conn, void* param, void* buffer, int buffer_size);

int xpsocket_init();
void xpsocket_cleanup();
int xpsocket_serve(xpsocket_receive_callback buffer_received, void* buffer_received_param, int buffer_size);
int xpsocket_send(xpsocket_handle conn, void* buffer, int size);
int xpsocket_receive(xpsocket_handle conn, void* buffer, int buffer_size, int* received_size);
xpsocket_handle xpsocket_init_client();
void xpsocket_free_client(xpsocket_handle conn);

#endif
