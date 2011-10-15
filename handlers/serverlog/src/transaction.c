/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak, Dylan Lukes
 * ServerLog handler library.
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include <mcproxy.h>
#include <gamestate.h>
#include <transaction.h>

int transact_handler_main(cid_t client_id, char direction, unsigned char msg_id,
			  nethost_t* hfrom, nethost_t* hto, objlist_t* data,
			  void* extra)
{
  int errcode;
  short transact_id;

  if(direction == MSG_TOSERVER || !gs_get_player())
    return PROXY_OK;

  transact_id = proto_gets(data, 1);
  if(proto_getc(data, 2) == 0) {
    gs_pop_transaction(transact_id);
    return PROXY_OK;
  }

  errcode = gs_call_transaction(client_id, transact_id, hto, hfrom);
  if(errcode > 0) {
    log_print(NULL, "(%04d) Transaction function %d returned error code: %d",
	      client_id, transact_id, errcode);
  }
  return PROXY_OK;
}
