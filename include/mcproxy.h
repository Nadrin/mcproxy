/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_H
#define __MCPROXY_H

#define MCPROXY_API

#include <config.h>
#include <network.h>

#include <proto.h>
#include <proxy.h>
#include <system.h>
#include <util.h>
#include <mm.h>
#include <thread.h>
#include <log.h>

// API
handler_info_t* handler_info(void);
int handler_startup(msgdesc_t* msglookup, event_t* events);
int handler_shutdown(void);

#endif
