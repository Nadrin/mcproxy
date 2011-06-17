/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 * No-op handler library.
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdlib.h>
#include <mcproxy.h>

char* handler_info(void)
{
  return "No-op v1.0";
}

int handler_startup(msgdesc_t* msglookup, event_t* events, unsigned long flags)
{
  return PROXY_OK;
}

int handler_shutdown(void)
{
  return PROXY_OK;
}
