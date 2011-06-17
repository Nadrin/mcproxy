/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 * ServerLog handler library.
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_HANDLERS_CHAT_H
#define __MCPROXY_HANDLERS_CHAT_H

struct chat_handler_config_s {
  gamestate_t* gs;
};
typedef struct chat_handler_config_s chat_handler_config_t;

void chat_init_config(chat_handler_config_t* cfg, gamestate_t* gs);

int chat_handler_main(cid_t client_id, char direction, unsigned char msg_id,
		      nethost_t* hfrom, nethost_t* hto, objlist_t* data,
		      void* extra);

#endif
