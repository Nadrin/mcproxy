/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_CORE_H
#define __MCPROXY_CORE_H

#include <proxy.h>

#define THREAD_FLAG_IDLE    0x00
#define THREAD_FLAG_RUNNING 0x01
#define THREAD_FLAG_CREATE  0x02
#define THREAD_FLAG_DEBUG   0x04

#define CORE_EOK            0x00
#define CORE_ENETWORK       0x01
#define CORE_EDATAHELPER    0x02
#define CORE_EHANDLER       0x03
#define CORE_EDONE          0x0A

// Handler Library API
typedef char* (*handler_info_func_t)(void);
typedef int   (*handler_startup_func_t)(msgdesc_t* msglookup, event_t* events, unsigned long flags);
typedef int   (*handler_shutdown_func_t)(void);

struct handler_api_s
{
  handler_info_func_t     handler_info;
  handler_startup_func_t  handler_startup;
  handler_shutdown_func_t handler_shutdown;
};
typedef struct handler_api_s handler_api_t;

struct thread_data_s
{
  int listen_sockfd;
  const char* server_addr;
  const char* server_port;
  volatile unsigned long flags;

  cid_t client_id;
  msgdesc_t* lookup;
  event_t*   events;
};
typedef struct thread_data_s thread_data_t;

// Core entry point
int core_main(const char* server_addr, const char* server_port, const char* listen_port,
	      int debug, handler_api_t* handler_api);

#endif
