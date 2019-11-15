#include "sock.h"

#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")

void translate_sockaddr(S_SOCKADDR* in, SOCKADDR* out)
{
	SOCKADDR_IN *tout = (SOCKADDR_IN*)out;
	if(in == NULL || out == NULL)
		return;
    tout->sin_family = AF_INET;
    tout->sin_port = in->port;
    tout->sin_addr.S_un.S_addr = in->ip;
}

void tranback_sockaddr(SOCKADDR* in, S_SOCKADDR* out)
{
    SOCKADDR_IN *tin = (SOCKADDR_IN*)in;
    if (in == NULL || out == NULL)
        return;
    out->family = tin->sin_family;
    out->ip = tin->sin_addr.S_un.S_addr;
    out->port = tin->sin_port;
}

unsigned short sock_htons(unsigned short hostshort)
{
	return htons(hostshort);
}

unsigned short sock_ntohs(unsigned short netshort)
{
	return ntohs(netshort);
}

unsigned int sock_htonl(unsigned long hostlong)
{
	return htonl(hostlong);
}

unsigned int sock_ntohl(unsigned long netlong)
{
	return ntohl(netlong);
}

unsigned int sock_inet_addr(char* cp)
{
	return inet_addr(cp);
}

WSADATA wsadata;

int sock_init()
{
    int ret = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (ret != ERROR_SUCCESS)
        return -1;
    else
        return 0;
}

int sock_finalize()
{
	WSACleanup();
    return 0;
}

S_SOCKET sock_socket_tcp_stream()
{
    SOCKET s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)
        return NULL;
    return (S_SOCKET)s;
}

int sock_closesocket(S_SOCKET socket)
{
	return closesocket((SOCKET)socket);
}

int sock_listen(S_SOCKET socket, int maxconn)
{
	return listen((SOCKET)socket, maxconn);
}

S_SOCKET sock_accept(S_SOCKET socket, S_SOCKADDR* fromaddr)
{
    int addrlen;
    SOCKET recvsocket;
    SOCKADDR sockaddr;
    addrlen = sizeof(SOCKADDR);
    recvsocket = accept((SOCKET)socket, &sockaddr, &addrlen);
    tranback_sockaddr(&sockaddr, fromaddr);
    if (recvsocket != INVALID_SOCKET)
        return (S_SOCKET)recvsocket;
    else
        return NULL;
}

int sock_bind(S_SOCKET socket, S_SOCKADDR* address)
{
	SOCKADDR sockaddr;
    memset(&sockaddr, 0, sizeof(SOCKADDR));
	translate_sockaddr(address, &sockaddr);
    if (bind((SOCKET)socket, &sockaddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
        return -1;
    return 0;
    //return -1 when error
}

int sock_recv(S_SOCKET socket, char* buf, UINT buflen, int flag)
{
	return recv((SOCKET)socket, buf, buflen, flag);
}

int sock_send(S_SOCKET socket, char* buf, UINT buflen, int flag)
{
	return send((SOCKET)socket, buf, buflen, flag);
}

int sock_set_timeout(S_SOCKET socket, int timeout)
{
    if (setsockopt((SOCKET)socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(int)) == SOCKET_ERROR)
        return -1;
    return 0;
}