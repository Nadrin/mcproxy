/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 * No-op proxy handler library.
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdlib.h>
#include <mcproxy.h>

handler_info_t* handler_info(void)
{
  static handler_info_t info = {
    "No-op", "Michał Siejak", 1, MCP_MODE_PROXY
  };
  return &info;
}

int handler_startup(msgdesc_t* msglookup, event_t* events)
{
  return SYSTEM_OK;
}

int handler_shutdown(void)
{
  return SYSTEM_OK;
}
