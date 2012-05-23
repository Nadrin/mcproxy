/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak, Dylan Lukes
 * ServerLog handler library.
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <glib.h>

#include <mcproxy.h>
#include <gamestate.h>
#include <track.h>

static void track_updateflags(player_t* player, char on_ground)
{
  if(on_ground == 1)
    player->flags |= GS_PLAYER_FLYING;
  else
    player->flags ^= GS_PLAYER_FLYING;
}

void track_init_config(track_handler_config_t* cfg, gamestate_t* gs)
{
  memset(cfg, 0, sizeof(track_handler_config_t));
  cfg->gs = gs;
}

int track_handler_main(cid_t client_id, char direction, unsigned char msg_id,
		       nethost_t* hfrom, nethost_t* hto, objlist_t* data,
		       void* extra)
{
  // Here by convention (not used for now)
  //track_handler_config_t* config = (track_handler_config_t*)extra;
  player_t* player = gs_get_player();

  if(!player) return PROXY_OK;
  
  switch(msg_id) {
  case 0x09:
    if(direction == MSG_TOCLIENT) {
      player->dim = proto_geti(data, 0) + GS_DIM_OFFSET;
      log_print(NULL, "(%04d) Player %s spawned in: %s", client_id,
		player->username, gs_get_dimstr(player->dim));
    }
    break;
  case 0x0A:
    track_updateflags(player, proto_getc(data, 0));
    break;
  case 0x0B:
    player->position[0] = proto_getd(data, 0);
    player->position[1] = proto_getd(data, 1);
    player->position[2] = proto_getd(data, 3);
    track_updateflags(player, proto_getc(data, 4));
    break;
  case 0x0C:
    player->look[0] = proto_getf(data, 0);
    player->look[1] = proto_getf(data, 1);
    track_updateflags(player, proto_getc(data, 2));
    break;
  case 0x0D:
    player->position[0] = proto_getd(data, 0);
    player->position[2] = proto_getd(data, 3);
    player->look[0] = proto_getf(data, 4);
    player->look[1] = proto_getf(data, 5);
    track_updateflags(player, proto_getc(data, 6));

    if(direction == MSG_TOSERVER)
      player->position[1] = proto_getd(data, 2);
    else
      player->position[1] = proto_getd(data, 1);
    break;
  }
  return PROXY_OK;
}

int track_handler_entity(cid_t client_id, char direction, unsigned char msg_id,
			 nethost_t* hfrom, nethost_t* hto, objlist_t* data,
			 void* extra)
{
  track_handler_config_t* config = (track_handler_config_t*)extra;

  GList*    entity_found;
  player_t* entity_player;
  char      entity_name[256];

  proto_getstr(data, 1, entity_name, 256);

  thread_mutex_lock(NULL);
  entity_found = g_list_find_custom(config->gs->playerdb, entity_name, gs_find_byname);
  if(entity_found) {
    entity_player = (player_t*)entity_found->data;
    if((entity_player->flags & GS_PLAYER_HIDDEN)) {
      thread_mutex_unlock(NULL);
      return PROXY_NOSEND;
    }
  }
  thread_mutex_unlock(NULL);
  return PROXY_OK;
}
