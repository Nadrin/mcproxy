/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 * ServerLog handler library.
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_HANDLER_GAMESTATE_H
#define __MCPROXY_HANDLER_GAMESTATE_H

#include <network.h>

#define GS_PLAYER_NORMAL 0x00
#define GS_PLAYER_FLYING 0x01
#define GS_PLAYER_OP     0x02
#define GS_PLAYER_HIDDEN 0x04

#define GS_DIM_OFFSET    2

#define GS_DIM_NETHER    0x01
#define GS_DIM_OVERWORLD 0x02
#define GS_DIM_END       0x03

#define GS_SLOT_OFFSET   9
#define GS_SLOT_MAX      40

#define GS_WINDOW_INVENTORY 0
#define GS_WINDOW_CHEST     1
#define GS_WINDOW_WORKBENCH 2
#define GS_WINDOW_FURNANCE  3
#define GS_WINDOW_DISPENSER 4
#define GS_WINDOW_ENCHANT   5

#define GS_OFFSET_WORKBENCH 10
#define GS_OFFSET_ENCHANT   1

// Item
struct item_s {
  short id;
  unsigned char count;
  unsigned short uses;
};
typedef struct item_s item_t;

// Player state
struct player_s {
  cid_t client_id;
  int entity_id;
  char username[50];
  double position[3];
  double look[2];
  unsigned long flags;
  unsigned long dim;
  unsigned short window;
  unsigned short invoffset;
  short hold;
  item_t mouse;
  unsigned short mouse_origin;
  item_t slots[GS_SLOT_MAX];
};
typedef struct player_s player_t;

// Transaction
typedef int (*transaction_func_t)(cid_t client_id, 
				  nethost_t* client, nethost_t* server, void* extra);
struct transaction_s {
  transaction_func_t function;
  unsigned short id;
  void* extra;
};
typedef struct transaction_s transaction_t;

// Gamestate
struct gamestate_s {
  GList* playerdb;
};
typedef struct gamestate_s gamestate_t;

// Initializers
player_t* gs_player_init(player_t* player,
			 const char* username,
			 cid_t cid, int eid);

item_t* gs_item_init(item_t* item,
		     short id,
		     unsigned char count,
		     unsigned short uses);

transaction_t* gs_transaction_init(transaction_t* transact,
				   unsigned short id,
				   transaction_func_t func,
				   void* extra);

// Find GCompare functions
gint gs_find_byname(gconstpointer a, gconstpointer b);
gint gs_find_bycid(gconstpointer a, gconstpointer b);
gint gs_find_byeid(gconstpointer a, gconstpointer b);

// TLS data accessors
void gs_set_player(player_t* player);
player_t* gs_get_player(void);

char* gs_get_dimstr(const int id);
void  gs_push_transaction(transaction_t* transact);
int   gs_pop_transaction(unsigned short id);
int   gs_call_transaction(cid_t client_id, unsigned short id,
			  nethost_t* client, nethost_t* server);

#endif
