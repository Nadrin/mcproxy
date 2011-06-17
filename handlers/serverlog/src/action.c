/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 * ServerLog handler library.
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include <mcproxy.h>
#include <gamestate.h>
#include <action.h>

void action_init_config(action_handler_config_t* cfg)
{
  g_datalist_init(&cfg->watchlist);
}

void action_free_config(action_handler_config_t* cfg)
{
  g_datalist_clear(&cfg->watchlist);
  g_datalist_init(&cfg->watchlist);
}

void action_set_watch(action_handler_config_t* cfg,
		      short id, const char* name)
{
  size_t name_len = strlen(name);
  char*  name_buf = g_malloc(name_len+1);
  memcpy(name_buf, name, name_len);
  name_buf[name_len] = 0;

  g_datalist_id_set_data_full(&cfg->watchlist, (GQuark)id, name_buf, g_free);
}

const char* action_get_watch(action_handler_config_t* cfg,
			     short id)
{
  return (const char*)g_datalist_id_get_data(&cfg->watchlist, (GQuark)id);
}

int action_handler_itemuse(cid_t client_id, char direction, unsigned char msg_id,
			   nethost_t* hfrom, nethost_t* hto, objlist_t* data,
			   void* extra)
{
  action_handler_config_t* config = (action_handler_config_t*)extra;

  player_t* player = gs_get_player();
  short item_id;
  const char* item_name;
  const char* action_name;
  int action_coords[3];

  if(!player) return PROXY_OK;

  switch(msg_id) {
  case 0x07: // Item use
    break;
  case 0x0F: // Block placement
    item_id = proto_gets(data, 4);
    if(item_id != -1) {
      item_name = action_get_watch(config, item_id);
      if(!item_name) break;

      action_coords[0] = proto_geti(data, 0);
      action_coords[1] = proto_getc(data, 1);
      action_coords[2] = proto_geti(data, 2);
      if(action_coords[0] == -1 && action_coords[1] == -1 && action_coords[2] == -1)
	break;

      if(item_id < 256) action_name = "placed";
      else action_name = "used";

      log_print(NULL, "(%04d) Player %s %s %s (%d) at %s:[%d,%d,%d]",
		client_id, player->username, action_name, item_name, item_id,
		gs_get_dimstr(player->dim),
		action_coords[0], action_coords[1], action_coords[2]);
    }
    break;
  case 0x10: // Holding change
    player->hold = proto_gets(data, 0);
    break;
  }
  return PROXY_OK;
}

int action_handler_explosion(cid_t client_id, char direction, unsigned char msg_id,
			     nethost_t* hfrom, nethost_t* hto, objlist_t* data,
			     void* extra)
{
  player_t* player = gs_get_player();
  if(!player || direction == MSG_TOSERVER)
    return PROXY_OK;

  log_print(NULL, "(%04d) Player %s at %s:[%.2f,%.2f,%.2f] heard an explosion at %s:[%.2f,%.2f,%.2f]",
	    client_id, player->username, 
	    gs_get_dimstr(player->dim),
	    player->position[0], player->position[1], player->position[2],
	    gs_get_dimstr(player->dim),
	    proto_getd(data, 0), proto_getd(data, 1), proto_getd(data, 2));

  return PROXY_OK;
}
