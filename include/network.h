/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak, Dylan Lukes
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_NETWORK_H
#define __MCPROXY_NETWORK_H

#include <stdint.h>
#include <arpa/inet.h>

#define NETOK  0
#define NETEOF 1
#define NETERR 2

struct nethost
{
  int s;
  uint16_t port;
  char addr[INET6_ADDRSTRLEN+1];
};

typedef struct nethost nethost_t;

nethost_t* net_hostinit(int s, const char* addr, unsigned short port);
int net_listen(const char* port, int* error);
nethost_t* net_connect(const char* addr, const char* port, int* error);
nethost_t* net_accept(int s);
void net_close(int s);
void net_free(nethost_t* nethost);

unsigned long net_select_rd(size_t count, ...);

int net_send(int s, const char* buf, size_t size);
int net_recv(int s, char* buf, size_t size);

#endif
