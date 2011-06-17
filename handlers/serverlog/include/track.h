/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 * ServerLog handler library.
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_HANDLER_TRACK_H
#define __MCPROXY_HANDLER_TRACK_H

struct track_handler_config_s {
  gamestate_t* gs;
};
typedef struct track_handler_config_s track_handler_config_t;

void track_init_config(track_handler_config_t* cfg, gamestate_t* gs);

int track_handler_main(cid_t client_id, char direction, unsigned char msg_id,
		       nethost_t* hfrom, nethost_t* hto, objlist_t* data,
		       void* extra);

int track_handler_entity(cid_t client_id, char direction, unsigned char msg_id,
			 nethost_t* hfrom, nethost_t* hto, objlist_t* data,
			 void* extra);

#endif
