/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 * ServerLog handler library.
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <glib.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#include <mcproxy.h>
#include <gamestate.h>

// Thread local storage
static __thread player_t* tls_player      = NULL;
static __thread GData* tls_transact_data  = NULL;

inline void gs_set_player(player_t* player)
{
  tls_player = player;
  if(!player) {
    g_datalist_clear(&tls_transact_data);
    g_datalist_init(&tls_transact_data);
  }
}

inline player_t* gs_get_player(void)
{
  return tls_player;
}

char* gs_get_dimstr(const int id)
{
  static char* names[] = { "world", "nether" };
  return names[id];
}

void gs_push_transaction(transaction_t* transact)
{
  transaction_t* _transact = g_malloc(sizeof(transaction_t));
  memcpy(_transact, transact, sizeof(transaction_t));
  g_datalist_id_set_data_full(&tls_transact_data, (GQuark)transact->id, _transact, g_free);
}

int gs_pop_transaction(unsigned short id)
{
  transaction_t* transact = g_datalist_id_get_data(&tls_transact_data,
						   (GQuark)id);
  if(!transact) return -1;

  if(transact->extra)
    free(transact->extra);
  g_datalist_id_remove_data(&tls_transact_data, (GQuark)id);
  return 0;
}

int gs_call_transaction(cid_t client_id, unsigned short id,
			nethost_t* client, nethost_t* server)
{
  int retval;
  transaction_t* transact = g_datalist_id_get_data(&tls_transact_data,
						   (GQuark)id);
  if(!transact) return -1;

  retval = transact->function(client_id, client, server, transact->extra);

  if(transact->extra)
    free(transact->extra);
  g_datalist_id_remove_data(&tls_transact_data, (GQuark)id);
  return retval;
}

gint gs_find_byname(gconstpointer a, gconstpointer b)
{
  return strncasecmp(((player_t*)a)->username, (const char*)b, 50);
}

gint gs_find_bycid(gconstpointer a, gconstpointer b)
{
  cid_t cid_a = ((player_t*)a)->client_id;
  cid_t cid_b = *(cid_t*)b;

  if(cid_a < cid_b)
    return -1;
  if(cid_a > cid_b)
    return 1;
  return 0;
}

gint gs_find_byeid(gconstpointer a, gconstpointer b)
{
  int eid_a = ((player_t*)a)->entity_id;
  int eid_b = *(cid_t*)b;

  if(eid_a < eid_b)
    return -1;
  if(eid_a > eid_b)
    return 1;
  return 0;
}

player_t* gs_player_init(player_t* player,
			 const char* username, cid_t cid, int eid)
{
  int i;
  memset(player, 0, sizeof(player_t));
  strncpy(player->username, username, 50);
  player->client_id = cid;
  player->entity_id = eid;
  player->invoffset = GS_SLOT_OFFSET;
  player->hold      = -1;
  player->mouse.id  = -1;
  for(i=0; i<GS_SLOT_MAX; i++)
    player->slots[i].id = -1;
  return player;
}

item_t* gs_item_init(item_t* item,
		     short id, unsigned char count, unsigned short uses)
{
  item->id    = id;
  item->count = count;
  item->uses  = uses;
  return item;
}

transaction_t* gs_transaction_init(transaction_t* transact,
				   unsigned short id,
				   transaction_func_t func,
				   void* extra)
{
  transact->id = id;
  transact->function = func;
  transact->extra  = extra;
  return transact;
}
