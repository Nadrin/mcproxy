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
    for(j=0; j<4; j++) { // Armor slots in inventory
      if(slot_id == 36 + j) {
	for(i=0; i<5; i++) if(item_id == 298 + j + i*4) return 1; // Armor parts
	if(j==0) if(item_id == 86) return 1; // Pumpkin
	return 0;
      }
    }
    break;
  case GS_WINDOW_WORKBENCH:
    if(slot_id == -10) // Cannot place anything in output slot
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

// WORK IN PROGRESS

#if 0
static void
inventory_reportchange(cid_t client_id, inventory_handler_config_t* config,
		       short id, short count, unsigned char type)
{
  player_t* player = gs_get_player();
  const char* item = inventory_get_watch(config, id);

  char* type_name  = "Chest";
  if(type == GS_WINDOW_INVENTORY ||
     type == GS_WINDOW_WORKBENCH ||
     type == GS_WINDOW_ENCHANT)
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
#endif

static void
inventory_autoplace(player_t* player, short item_id, short count)
{
  
}

static int
inventory_transact_windowclick(cid_t client_id,
                               nethost_t* client, nethost_t* server, void* extra)
{
  inventory_clickdata_t* click_data = (inventory_clickdata_t*)extra;
  player_t* player = gs_get_player();
  slot_t* s = &click_data->slotdata;
  
  short slot_id = -1;
  //short save_count = 0;

  if(click_data->window == GS_WINDOW_INVENTORY)
    player->invoffset = GS_SLOT_OFFSET;

  if(click_data->slot >= 0)
    slot_id = inventory_get_invslot(click_data->slot, player->invoffset, click_data->window);

  if(player->mouse.id == -1) { // Hand is empty
    if(click_data->slot < 0 || s->id == -1) // Clicked outside of window or at empty slot.
      return PROXY_OK;

    if(click_data->flags & INV_FLAG_SHIFT) { // Holding shift
      if(slot_id >= 0) // Clicked inside player's inventory
        player->slots[slot_id].count = 0; // Item removed from slot
      else
        inventory_autoplace(player, s->id, s->count); // Autoplace item in quickbar
    }
    else {
      player->mouse.id = s->id;
      player->mouse.uses = s->meta;
    
      if(click_data->flags & INV_FLAG_RIGHT) // Clicked with right mouse button
        player->mouse.count = ceilf(s->count * 0.5f);
      else // Clicked with left mouse button
        player->mouse.count = s->count;

      if(slot_id >= 0) // Clicked inside player's inventory
        player->slots[slot_id].count -= player->mouse.count; // Remove items from inventory
      else
        player->mouse_origin = click_data->window;
    }
  }
  else { // Hand is holding an item
    if(click_data->slot < 0) { // Clicked outside the window
      if(click_data->flags & INV_FLAG_RIGHT) // Clicked with right mouse button
        player->mouse.count--;
      else
        player->mouse.count = 0;
    }
    else { // Clicked inside the window
      if(click_data->flags & INV_FLAG_SHIFT) // Holding shift
        return PROXY_OK;
      if(!inventory_validate(slot_id, player->mouse.id, player->window)) // Player selected invalid slot
        return PROXY_OK;

      
    }
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
  short slot_id;
  slot_t slotdata;

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
      player->invoffset = GS_OFFSET_WORKBENCH;
    if(player->window == GS_WINDOW_ENCHANT)
      player->invoffset = GS_OFFSET_ENCHANT;
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
    click_data->flags      = INV_FLAG_NONE;

    if(proto_getc(data, 2))
      click_data->flags |= INV_FLAG_RIGHT;
    if(proto_getc(data, 4))
      click_data->flags |= INV_FLAG_SHIFT;

    proto_getslot(proto_list(data, 5), 0, &click_data->slotdata);

    gs_transaction_init(&click_transact, proto_gets(data, 3),
			inventory_transact_windowclick,
			click_data);

    gs_push_transaction(&click_transact);
    break;
  case 0x67: // Set slot
    if(direction == MSG_TOSERVER)
      break;
    if(proto_getc(data, 0) != GS_WINDOW_INVENTORY)
      break;
    
    slot_id = proto_gets(data, 1);
    proto_getslot(proto_list(data, 2), 0, &slotdata);

    slot_id = inventory_get_invslot(slot_id, player->invoffset, player->window);
    if(slotdata.id == -1) {
      gs_item_init(&player->slots[slot_id], -1, 0, 0);
    }
    else {
      gs_item_init(&player->slots[slot_id],
		   slotdata.id, slotdata.count, slotdata.meta);
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
  objlist_t* list;
  
  player_t* player = gs_get_player();

  if(!player || direction == MSG_TOSERVER)
    return PROXY_OK;
  if(proto_getc(data, 0) != GS_WINDOW_INVENTORY)
    return PROXY_OK;

  list = proto_list(data, 2);
  for(i=0; i<proto_gets(data, 1); i++) {
    slot_t slotdata;
    proto_getslot(&list[i], 0, &slotdata);
    
    slot_id = inventory_get_invslot(i, GS_SLOT_OFFSET, GS_WINDOW_INVENTORY);
    if(slotdata.id == -1) {
      if(slot_id > 0)
	gs_item_init(&player->slots[slot_id-1], -1, 0, 0);
      continue;
    }

    if(slot_id > 0) {
      gs_item_init(&player->slots[slot_id-1],
                   slotdata.id, slotdata.count, slotdata.meta);
    }
  }
  return PROXY_OK;
}
