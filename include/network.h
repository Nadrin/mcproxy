/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_NETWORK_H
#define __MCPROXY_NETWORK_H

#define NETOK  0
#define NETEOF 1
#define NETERR 2

struct nethost
{
  int s;
  char addr[100];
  unsigned short port;
};

typedef struct nethost nethost_t;

nethost_t* net_hostinit(int s, const char* addr, unsigned short port);
int net_listen(unsigned short port);
nethost_t* net_connect(const char* addr, unsigned short port);
nethost_t* net_accept(int s);
void net_close(int s);
unsigned long net_select_rd(size_t count, ...);

int net_send(int s, const char* buf, size_t size);
int net_recv(int s, char* buf, size_t size);

#endif
