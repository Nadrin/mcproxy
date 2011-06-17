/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 * ServerLog handler library.
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_HANDLERS_INVENTORY_H
#define __MCPROXY_HANDLERS_INVENTORY_H

struct inventory_handler_config_s {
  GData* watchlist;
};
typedef struct inventory_handler_config_s inventory_handler_config_t;

struct inventory_clickdata_s {
  unsigned char window;
  short slot;
  char right;
  short item_id;
  unsigned char item_count;
  unsigned short item_uses;
  inventory_handler_config_t* config;
};
typedef struct inventory_clickdata_s inventory_clickdata_t;

void inventory_init_config(inventory_handler_config_t* cfg);
void inventory_free_config(inventory_handler_config_t* cfg);
void inventory_set_watch(inventory_handler_config_t* cfg,
			 short id, const char* name);
const char* inventory_get_watch(inventory_handler_config_t* cfg,
				short id);

int inventory_handler_main(cid_t client_id, char direction, unsigned char msg_id,
			   nethost_t* hfrom, nethost_t* hto, objlist_t* data,
			   void* extra);

int inventory_handler_windowitems(cid_t client_id, char direction, unsigned char msg_id,
				  nethost_t* hfrom, nethost_t* hto, objlist_t* data,
				  void* extra);

#endif
