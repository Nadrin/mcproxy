/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 * Namechange proxy handler library.
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdlib.h>
#include <mcproxy.h>

static int handler_loginrequest(cid_t client_id, char direction, unsigned char msg_id,
				nethost_t* hfrom, nethost_t* hto, objlist_t* data,
				void* extra)
{
  char* player_name = (char*)extra;
  if(direction == MSG_TOSERVER)
    proto_putstr(data, 1, player_name);
  return PROXY_OK;
}

handler_info_t* handler_info(void)
{
  static handler_info_t info = {
    "NameChange", "Michał Siejak", 1, MODE_TYPE_PROXY
  };
  return &info;
}

int handler_startup(msgdesc_t* msglookup, event_t* events)
{
  if(sys_argc() != 1) {
    log_print(NULL, "NameChange: Invalid arguments");
    return SYSTEM_ERROR;
  }

  proxy_register(msglookup, 0x01, handler_loginrequest, sys_argv()[0], NULL, NULL);
  log_print(NULL, "NameChange: Player will report as \"%s\" to the server", sys_argv()[0]);
  return SYSTEM_OK;
}

int handler_shutdown(void)
{
  return SYSTEM_OK;
}
