/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak, Dylan Lukes
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
#include <errno.h>

#include <config.h>
#include <network.h>

static inline void* net_get_inetaddr(struct sockaddr* addr)
{
  if(addr->sa_family == AF_INET6)
    return &(((struct sockaddr_in6*)addr)->sin6_addr);
  else
    return &(((struct sockaddr_in*)addr)->sin_addr);
}

static inline uint16_t net_get_inetport(struct sockaddr* addr)
{
  if(addr->sa_family == AF_INET6)
    return (uint16_t)((struct sockaddr_in6*)addr)->sin6_port;
  else
    return (uint16_t)((struct sockaddr_in*)addr)->sin_port;
}

nethost_t* net_hostinit(int s, const char* addr, unsigned short port)
{
  nethost_t* result = malloc(sizeof(nethost_t));
  result->s    = s;
  result->port = port;
  strncpy(result->addr, addr, INET6_ADDRSTRLEN);
  return result;
}

int net_listen(const char* port, int* error)
{
  const int _sockopt_yes = 1;
  const int _sockopt_no  = 0;

  int errval = 0;
  int s;

  struct addrinfo hints, *servinfo, *p;
  if(error) *error = 0;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags    = AI_PASSIVE;

  if((errval = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
    if(error) *error = errval;
    return 0;
  }

  for(p=servinfo; p!=NULL; p=p->ai_next) {
    s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if(s == -1) continue;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &_sockopt_yes, sizeof(int));
    if(p->ai_family == AF_INET6)
      setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &_sockopt_no, sizeof(int));

    if(bind(s, p->ai_addr, p->ai_addrlen) == -1) {
      close(s);
      continue;
    }
    break;
  }
  freeaddrinfo(servinfo);
  if(!p) return 0;

  listen(s, MCPROXY_MAXQUEUE);
  return s;
}

nethost_t* net_connect(const char* addr, const char* port, int* error)
{
  int errval = 0;
  nethost_t* retval;
  int s;

  struct addrinfo hints, *servinfo, *p;
  char addrbuf[INET6_ADDRSTRLEN+1];
  if(error) *error = 0;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if((errval = getaddrinfo(addr, port, &hints, &servinfo)) != 0) {
    if(error) *error = errval;
    return NULL;
  }
  
  for(p=servinfo; p!=NULL; p=p->ai_next) {
    s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if(s == -1) continue;
    
    if(connect(s, p->ai_addr, p->ai_addrlen) == -1) {
      close(s);
      continue;
    }
    break;
  }
  if(!p) {
    freeaddrinfo(servinfo);
    return NULL;
  }

  if(inet_ntop(p->ai_family,
	       net_get_inetaddr((struct sockaddr*)p->ai_addr),
	       addrbuf, sizeof(addrbuf)) == NULL) {
    freeaddrinfo(servinfo);
    close(s);
    return NULL;
  }

  retval = net_hostinit(s, addrbuf, net_get_inetport((struct sockaddr*)p->ai_addr));
  freeaddrinfo(servinfo);
  return retval;
}

nethost_t* net_accept(int s)
{
  socklen_t addrlen;
  char addrbuf[INET6_ADDRSTRLEN+1];
  struct sockaddr_storage hostaddr;
  int cs;

  addrlen = sizeof(struct sockaddr_storage);
  cs = accept(s, (struct sockaddr*)&hostaddr, &addrlen);
  if(cs < 0) return NULL;

  if(inet_ntop(hostaddr.ss_family,
	       net_get_inetaddr((struct sockaddr*)&hostaddr),
	       addrbuf, sizeof(addrbuf)) == NULL) {
    close(cs);
    return NULL;
  }
    
  return net_hostinit(cs, addrbuf, net_get_inetport((struct sockaddr*)&hostaddr));
}

void net_close(int s)
{
  shutdown(s, SHUT_RDWR);
  close(s);
}

void net_free(nethost_t* nethost)
{
  net_close(nethost->s);
  free(nethost);
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

  do {
    ret = send(s, buf + size - rem, rem, 0);
    if(ret < 0 && errno == EINTR)
      continue;
    if(ret > 0) rem -= ret;
  } while(ret > 0 && rem > 0);

  if(ret == 0)  return NETEOF;
  if(ret == -1) return NETERR;
  return NETOK;
}

int net_recv(int s, char* buf, size_t size)
{
  ssize_t rem = size;
  ssize_t ret;

  do {
    ret = recv(s, buf + size - rem, rem, 0);
    if(ret < 0 && errno == EINTR)
      continue;
    if(ret > 0) rem -= ret;
  } while(ret > 0 && rem > 0);

  if(ret == 0)  return NETEOF;
  if(ret == -1) return NETERR;
  return NETOK;
}
