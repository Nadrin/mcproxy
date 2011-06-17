/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 * ServerLog handler library.
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_HANDLER_ACTION_H
#define __MCPROXY_HANDLER_ACTION_H

struct action_handler_config_s {
  GData* watchlist;
};
typedef struct action_handler_config_s action_handler_config_t;

void action_init_config(action_handler_config_t* cfg);
void action_free_config(action_handler_config_t* cfg);

void action_set_watch(action_handler_config_t* cfg,
		      short id, const char* name);

const char* action_get_watch(action_handler_config_t* cfg,
			     short id);

int action_handler_itemuse(cid_t client_id, char direction, unsigned char msg_id,
			   nethost_t* hfrom, nethost_t* hto, objlist_t* data,
			   void* extra);

int action_handler_explosion(cid_t client_id, char direction, unsigned char msg_id,
			     nethost_t* hfrom, nethost_t* hto, objlist_t* data,
			     void* extra);

#endif
