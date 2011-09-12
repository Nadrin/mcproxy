/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_SYSTEM_H
#define __MCPROXY_SYSTEM_H

#include <proxy.h>

#define SYSTEM_OK        0x00
#define SYSTEM_SHUTDOWN  0xFE
#define SYSTEM_ERROR     0xFF

#define MODE_TYPE_UNSPEC 0x00
#define MODE_TYPE_PROXY  0x01
#define MODE_TYPE_CLIENT 0x02
#define MODE_TYPE_SERVER 0x03
#define MODE_TYPE_MAX    0x04

// Handler Library API
struct handler_info_s
{
  char name[100];
  char author[100];
  int  version;
  int  type;
};
typedef struct handler_info_s handler_info_t;

typedef handler_info_t* (*handler_info_func_t)(void);
typedef int             (*handler_startup_func_t)(msgdesc_t* msglookup, event_t* events);
typedef int             (*handler_shutdown_func_t)(void);

struct handler_api_s
{
  handler_info_func_t     handler_info;
  handler_startup_func_t  handler_startup;
  handler_shutdown_func_t handler_shutdown;
};
typedef struct handler_api_s handler_api_t;

// Global system struct
struct sys_config_s
{
  char server_addr[256];
  char listen_port[100];
  char server_port[100];
  char logfile[PATH_MAX];
  char pidfile[PATH_MAX];
  char libfile[PATH_MAX];
  int  debug_flag;
  unsigned long connect_delay;
};
typedef struct sys_config_s sys_config_t;

// System API
int           sys_initapi(void* library, handler_api_t* handler_api);
int           sys_status(void);
sys_config_t* sys_get_config(void);
unsigned int  sys_get_mode(void);
int           sys_set_mode(unsigned int mode);
void          sys_set_args(int argc, char** argv);

int           sys_argc(void);
char**        sys_argv(void);

#endif
