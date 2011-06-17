/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <config.h>
#include <network.h>

nethost_t* net_hostinit(int s, const char* addr, unsigned short port)
{
  nethost_t* result = malloc(sizeof(nethost_t));
  result->s    = s;
  result->port = port;
  strcpy(result->addr, addr);
  return result;
}

int net_listen(unsigned short port)
{
  struct sockaddr_in bindaddr;
  int s;

  memset(&bindaddr, 0, sizeof(struct sockaddr_in));
  bindaddr.sin_family      = AF_INET;
  bindaddr.sin_port        = htons(port);
  bindaddr.sin_addr.s_addr = INADDR_ANY;
  
  s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(bind(s, (struct sockaddr*)&bindaddr, sizeof(struct sockaddr_in)) < 0) {
    close(s);
    return 0;
  }
  listen(s, MCPROXY_MAXQUEUE);
  return s;
}

nethost_t* net_connect(const char* addr, unsigned short port)
{
  struct sockaddr_in servaddr;
  struct hostent* server;
  int s;

  server = gethostbyname(addr);
  if(!server) return NULL;

  s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  memset(&servaddr, 0, sizeof(struct sockaddr_in));
  memcpy((char*)&servaddr.sin_addr.s_addr, server->h_addr, server->h_length);
  servaddr.sin_family = AF_INET;
  servaddr.sin_port   = htons(port);

  if(connect(s, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_in)) < 0)
    return NULL;

  return net_hostinit(s, inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));
}

nethost_t* net_accept(int s)
{
  socklen_t addrlen;
  struct sockaddr_in hostaddr;
  int cs;

  addrlen = sizeof(struct sockaddr_in);
  cs = accept(s, (struct sockaddr*)&hostaddr, &addrlen);
  if(cs < 0) return NULL;

  return net_hostinit(cs, inet_ntoa(hostaddr.sin_addr), ntohs(hostaddr.sin_port));
}

void net_close(int s)
{
  shutdown(s, SHUT_RDWR);
  close(s);
}

unsigned long net_select_rd(size_t count, ...)
{
  fd_set readfs;
  va_list ap;
  size_t i;

  int  maxfd  = 0;
  int  retval = 0;
  int  rdbuf[MCPROXY_SELECT_MAX];

  if(count > MCPROXY_SELECT_MAX)
    return 0;

  FD_ZERO(&readfs);
  va_start(ap, count);
  for(i=0; i<count; i++) {
    rdbuf[i] = va_arg(ap, int);
    FD_SET(rdbuf[i], &readfs);
    if(rdbuf[i] > maxfd)
      maxfd = rdbuf[i];
  }
  va_end(ap);

  if(select(maxfd+1, &readfs, NULL, NULL, NULL) <= 0)
    return 0;

  for(i=0; i<count; i++) {
    if(FD_ISSET(rdbuf[i], &readfs))
      retval |= (1 << (unsigned long)i);
  }
  return retval;
}

int net_send(int s, const char* buf, size_t size)
{
  ssize_t rem = size;
  ssize_t ret;

  while((ret = send(s, buf + size - rem, rem, 0)) > 0) {
    rem -= ret;
    if(rem == 0) break;
  }

  if(ret == 0)  return NETEOF;
  if(ret == -1) return NETERR;
  return NETOK;
}

int net_recv(int s, char* buf, size_t size)
{
  ssize_t rem = size;
  ssize_t ret;

  while((ret = recv(s, buf + size - rem, rem, 0)) > 0) {
    rem -= ret;
    if(rem == 0) break;
  }
  if(ret == 0)  return NETEOF;
  if(ret == -1) return NETERR;
  return NETOK;
}
