#ifndef SOCK_H
#define SOCK_H

typedef unsigned int S_UINT;
typedef unsigned int* S_SOCKET;

typedef struct sock_sockaddr{
	unsigned short family;
	unsigned short port;
	unsigned long ip;
	
} S_SOCKADDR;

unsigned short sock_htons(unsigned short hostshort);

unsigned short sock_ntohs(unsigned short netshort);

unsigned int sock_htonl(unsigned long hostlong);

unsigned int sock_ntohl(unsigned long netlong);

unsigned int sock_inet_addr(char* cp);

int sock_init();

int sock_finalize();

S_SOCKET sock_socket_tcp_stream();
//return NULL when error

int sock_closesocket(S_SOCKET socket);

int sock_listen(S_SOCKET socket, int maxconn);

S_SOCKET sock_accept(S_SOCKET socket, S_SOCKADDR* fromaddr);
//return NULL when error

int sock_bind(S_SOCKET socket, S_SOCKADDR* address);
//return -1 when error

int sock_recv(S_SOCKET socket, char* buf, S_UINT buflen, int flag);

int sock_send(S_SOCKET socket, char* buf, S_UINT buflen, int flag);

int sock_set_timeout(S_SOCKET socket, int timeout);
//return -1 when error

#endif