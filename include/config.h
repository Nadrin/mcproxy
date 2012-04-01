/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak, Dylan Lukes
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_CONFIG_H
#define __MCPROXY_CONFIG_H

#include <limits.h>
#include <platform.h>

#define MCPROXY_VERSION_MAJOR 0
#define MCPROXY_VERSION_MINOR 3
#define MCPROXY_VERSION_STR   "0.3.0"
#define MCPROXY_PROTOCOL      18

#define MCPROXY_LOGFILE       "mcproxy.log"
#define MCPROXY_PIDFILE       "mcproxy.pid"
#define MCPROXY_DEFAULT_HOST  "localhost"
#define MCPROXY_DEFAULT_PORT  "25565"
#define MCPROXY_DEFAULT_LPORT "25566"
#define MCPROXY_DEFAULT_DELAY 5000

#define MCPROXY_MAXQUEUE     10
#define MCPROXY_META_MAX     64
#define MCPROXY_SELECT_MAX   16

#define MCPROXY_THREAD_STACK 524288
#define MCPROXY_THREAD_POOL  524288

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// Some built-in types
typedef unsigned long cid_t;
typedef unsigned char bool;

#endif
