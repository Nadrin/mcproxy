/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak, Dylan Lukes
 * ServerLog handler library.
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <arpa/inet.h>

#include <mcproxy.h>
#include <gamestate.h>
#include <inventory.h>

static int
inventory_validate(short slot_id, short item_id, unsigned char window)
{
  int i,j;

  switch(window) {
  case GS_WINDOW_INVENTORY:
    if(slot_id == -9)
      return 0;
    for(j=0; j<4; j++) {
      if(slot_id == 36 + j) {
	for(i=0; i<5; i++) if(item_id == 298 + j + i*4) return 1;
	if(j==0) if(item_id == 86) return 1;
	return 0;
      }
    }
    break;
  case GS_WINDOW_WORKBENCH:
    if(slot_id == -10)
      return 0;
    break;
  }
  return 1;
}

static inline short
inventory_get_invslot(short i, short offset, unsigned char window)
{
  short slot_id = i - offset;
  if(window == GS_WINDOW_INVENTORY) {
    if(slot_id < 0 && slot_id >= -4) // Armor
      slot_id += GS_SLOT_MAX;
  }
  return slot_id;
}

static void
inventory_reportchange(cid_t client_id, inventory_handler_config_t* config,
		       short id, short count, unsigned char type)
{
  player_t* player = gs_get_player();
  const char* item = inventory_get_watch(config, id);

  char* type_name  = "Chest";
  if(type == GS_WINDOW_INVENTORY || type == GS_WINDOW_WORKBENCH)
    return;
  
  if(type == GS_WINDOW_FURNANCE)
    type_name = "Furnance";
  else if(type == GS_WINDOW_DISPENSER)
    type_name = "Dispenser";

  if(item) {
    log_print(NULL, "(%04d) Player %s took %dx %s (%d) from %s near %s:[%.2f,%.2f,%.2f]",
	      client_id, player->username, count, item, id, type_name,
	      gs_get_dimstr(player->dim),
	      player->position[0], player->position[1], player->position[2]);
  }
}

static int 
inventory_transact_windowclick(cid_t client_id,
			       nethost_t* client, nethost_t* server, void *extra)
{
  inventory_clickdata_t* click_data = (inventory_clickdata_t*)extra;
  player_t* player = gs_get_player();
  short slot_id = -1;
  short save_count = 0;

  // Notch, I hate you for this. :<
  if(click_data->window == 0)
    player->invoffset = GS_SLOT_OFFSET;

  if(click_data->slot >= 0)
    slot_id = inventory_get_invslot(click_data->slot, player->invoffset, click_data->window);
  
  if(player->mouse.id == -1) {
    if(click_data->slot < 0 || click_data->item_id == -1)
      return PROXY_OK;

    player->mouse.id = click_data->item_id;
    player->mouse.uses = click_data->item_uses;

    if(click_data->right) {
      player->mouse.count = ceilf(click_data->item_count / 2.0f);
      if(slot_id >= 0)
	player->slots[slot_id].count -= player->mouse.count;
      else
	player->mouse_origin = click_data->window;
    }
    else {
      player->mouse.count = click_data->item_count;
      if(slot_id >= 0)
	player->slots[slot_id].count = 0;
      else
	player->mouse_origin = click_data->window;
    }
  }
  else {
    save_count = player->mouse.count;
    if(click_data->slot < 0) {
      if(click_data->right)
	player->mouse.count--;
      else
	player->mouse.count = 0;
      slot_id = 0;
    }
    else {
      if(inventory_validate(slot_id, player->mouse.id, player->window) == 0)
	return PROXY_OK;

      if(slot_id >= 0) {
	player->slots[slot_id].id = player->mouse.id;
	player->slots[slot_id].uses = player->mouse.uses;
      }
      if(click_data->item_id == player->mouse.id) {
	if(click_data->right) {
	  if(slot_id >= 0)
	    player->slots[slot_id].count++;
	  player->mouse.count--;
	}
	else {
	  if(slot_id >= 0)
	    player->slots[slot_id].count += player->mouse.count;
	  player->mouse.count = 0;
	}
      }
      else {
	if(click_data->item_id == -1) {
	  if(click_data->right) {
	    if(slot_id >= 0)
	      player->slots[slot_id].count = 1;
	    player->mouse.count--;
	  }
	  else {
	    if(slot_id >= 0)
	      player->slots[slot_id].count = player->mouse.count;
	    player->mouse.count = 0;
	  }
	}
	else {
	  if(slot_id >= 0)
	    player->slots[slot_id].count = player->mouse.count;
	  player->mouse.id    = click_data->item_id;
	  player->mouse.count = click_data->item_count;
	  player->mouse.uses  = click_data->item_uses;
	}
      }
    }
  }

  if(slot_id >= 0 && player->mouse_origin != GS_WINDOW_INVENTORY)
    inventory_reportchange(client_id, click_data->config,
			   player->mouse.id, save_count, player->window);
 
  if(player->mouse.count == 0) {
    player->mouse.id = -1;
    player->mouse.uses = 0;
    player->mouse_origin = GS_WINDOW_INVENTORY;
  }
  if(slot_id >= 0 && player->slots[slot_id].count == 0) {
    player->slots[slot_id].id = -1;
    player->slots[slot_id].uses = 0;
  }

  return PROXY_OK;
}

void inventory_init_config(inventory_handler_config_t* cfg)
{
  g_datalist_init(&cfg->watchlist);
}

void inventory_free_config(inventory_handler_config_t* cfg)
{
  g_datalist_clear(&cfg->watchlist);
  g_datalist_init(&cfg->watchlist);
}

void inventory_set_watch(inventory_handler_config_t* cfg,
			 short id, const char* name)
{
  size_t name_len = strlen(name);
  char*  name_buf = g_malloc(name_len+1);
  memcpy(name_buf, name, name_len);
  name_buf[name_len] = 0;

  g_datalist_id_set_data_full(&cfg->watchlist, (GQuark)id, name_buf, g_free);
}

const char* inventory_get_watch(inventory_handler_config_t* cfg,
				short id)
{
  return (const char*)g_datalist_id_get_data(&cfg->watchlist, (GQuark)id);
}

int inventory_handler_main(cid_t client_id, char direction, unsigned char msg_id,
			   nethost_t* hfrom, nethost_t* hto, objlist_t* data,
			   void* extra)
{
  short item_id;
  short slot_id;

  transaction_t click_transact;
  inventory_clickdata_t* click_data;

  inventory_handler_config_t* config = (inventory_handler_config_t*)extra;
  player_t* player = gs_get_player();
  if(!player) return PROXY_OK;

  switch(msg_id) {
  case 0x64: // Open window
    if(direction == MSG_TOSERVER)
      break;
    player->window = proto_getc(data, 1) + 1;
    player->invoffset = proto_getc(data, 3);
    if(player->window == GS_WINDOW_WORKBENCH)
      player->invoffset++;
    break;
  case 0x65: // Close window
    player->window = GS_WINDOW_INVENTORY;
    player->invoffset = GS_SLOT_OFFSET;
    break;
  case 0x66: // Window click
    click_data = malloc(sizeof(inventory_clickdata_t));
    click_data->config     = config;
    click_data->window     = proto_getc(data, 0);
    click_data->slot       = proto_gets(data, 1);
    click_data->right      = proto_getc(data, 2);
    click_data->item_id    = proto_gets(data, 5);

    if(click_data->item_id != -1) {
      click_data->item_count = proto_getc(data, 6);
      click_data->item_uses  = proto_gets(data, 7);
    }

    gs_transaction_init(&click_transact, proto_gets(data, 3),
			inventory_transact_windowclick,
			click_data);

    gs_push_transaction(&click_transact);
    break;
  case 0x67: // Set slot
    if(direction == MSG_TOSERVER)
      break;
    if(proto_getc(data, 0) != 0)
      break;
    slot_id = proto_gets(data, 1);
    item_id = proto_gets(data, 2);

    slot_id = inventory_get_invslot(slot_id, player->invoffset, player->window);
    if(item_id == -1) {
      gs_item_init(&player->slots[slot_id], -1, 0, 0);
    }
    else {
      gs_item_init(&player->slots[slot_id],
		   item_id, proto_getc(data, 3), proto_gets(data, 4));
    }
    break;
  }
  return PROXY_OK;
}

int inventory_handler_windowitems(cid_t client_id, char direction, unsigned char msg_id,
				  nethost_t* hfrom, nethost_t* hto, objlist_t* data,
				  void* extra)
{
  short i, slot_id;
  unsigned char* dataptr;
  player_t* player = gs_get_player();

  if(!player || direction == MSG_TOSERVER)
    return PROXY_OK;

  if(proto_getc(data, 0) != GS_WINDOW_INVENTORY)
    return PROXY_OK;

  dataptr = (unsigned char*)data->dataptr;
  for(i=0; i<proto_gets(data, 1); i++) {
    unsigned char count;
    short uses;
    short item_id = ntohs(*(short*)dataptr);
    dataptr += 2;

    slot_id = inventory_get_invslot(i, GS_SLOT_OFFSET, GS_WINDOW_INVENTORY);
    if(item_id == -1) {
      if(slot_id > 0)
	gs_item_init(&player->slots[slot_id-1], -1, 0, 0);
      continue;
    }
    count = *dataptr++;
    uses  = ntohs(*(short*)dataptr);
    dataptr += 2;

    if(slot_id > 0) {
      gs_item_init(&player->slots[slot_id-1], item_id, count, uses);
    }
  }
  return PROXY_OK;
}
