/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_CONFIG_H
#define __MCPROXY_CONFIG_H

#define MCPROXY_LOGFILE       "mcproxy.log"
#define MCPROXY_PIDFILE       "mcproxy.pid"
#define MCPROXY_DEFAULT_HOST  "localhost"
#define MCPROXY_DEFAULT_PORT  "25565"
#define MCPROXY_DEFAULT_LPORT "25566"

#define MCPROXY_MAXQUEUE     10
#define MCPROXY_THROTTLE     5000
#define MCPROXY_META_MAX     64
#define MCPROXY_SELECT_MAX   16

#define MCPROXY_THREAD_STACK 524288
#define MCPROXY_THREAD_POOL  524288

#ifndef PATH_MAX
#define PATH_MAX 256
#endif

// Some built-in types
typedef unsigned long cid_t;

#endif
