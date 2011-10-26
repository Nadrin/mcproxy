/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak, Dylan Lukes
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <config.h>
#include <log.h>
#include <proxy.h>
#include <system.h>
#include <mm.h>
#include <util.h>

extern msgdesc_t msgtable[];

static int
helper_generic_item(cid_t client_id, char mode, unsigned char msg_id, nethost_t* host, objlist_t* data, void* extra)
{
  int base_index = 4 + (msg_id == 0x66);

  if(proto_gets(data, base_index) < 0)
    return PROXY_OK;

  if(mode == MODE_RECV) {
    if(proto_recv_object(host, &data->objects[base_index+1], 'c') != 0) return 1;
    if(proto_recv_object(host, &data->objects[base_index+2], 's') != 0) return 1;
  }
  else {
    if(proto_send_object(host, &data->objects[base_index+1]) != 0) return 1;
    if(proto_send_object(host, &data->objects[base_index+2]) != 0) return 1;
  }
  return PROXY_OK;
}

static int
helper_add_object(cid_t client_id, char mode, unsigned char msg_id, nethost_t* host, objlist_t* data, void* extra)
{
  if(proto_geti(data, 5) == 0)
    return PROXY_OK;

  if(mode == MODE_RECV) {
    if(proto_recv_object(host, &data->objects[6], 's') != 0) return 1;
    if(proto_recv_object(host, &data->objects[7], 's') != 0) return 1;
    if(proto_recv_object(host, &data->objects[8], 's') != 0) return 1;
  }
  else {
    if(proto_send_object(host, &data->objects[6]) != 0) return 1;
    if(proto_send_object(host, &data->objects[7]) != 0) return 1;
    if(proto_send_object(host, &data->objects[8]) != 0) return 1;
  }
  return PROXY_OK;
}

static int
helper_set_slot(cid_t client_id, char mode, unsigned char msg_id, nethost_t* host, objlist_t* data, void* extra)
{
  if(proto_gets(data, 2) < 0)
    return PROXY_OK;

  if(mode == MODE_RECV) {
    if(proto_recv_object(host, &data->objects[3], 'c') != 0) return 1;
    if(proto_recv_object(host, &data->objects[4], 's') != 0) return 1;
  }
  else {
    if(proto_send_object(host, &data->objects[3]) != 0) return 1;
    if(proto_send_object(host, &data->objects[4]) != 0) return 1;
  }
  return PROXY_OK;
}

static int
helper_map_chunk(cid_t client_id, char mode, unsigned char msg_id, nethost_t* host, objlist_t* data, void* extra)
{
  size_t datasize = proto_geti(data, 6);
  return proxy_transfer(mode, host, data, datasize);
}

static int
helper_multiblock_change(cid_t client_id, char mode, unsigned char msg_id, nethost_t* host, objlist_t* data, void* extra)
{
  short count = proto_gets(data, 2);
  size_t datasize = count * 4;
  return proxy_transfer(mode, host, data, datasize);
}

static int
helper_explosion(cid_t client_id, char mode, unsigned char msg_id, nethost_t* host, objlist_t* data, void* extra)
{
  size_t datasize = proto_geti(data, 4) * 3;
  if(datasize == 0) return PROXY_OK;
  return proxy_transfer(mode, host, data, datasize);
}

static int
helper_window_items(cid_t client_id, char mode, unsigned char msg_id, nethost_t* host, objlist_t* data, void* extra)
{
  short i;
  short count = proto_gets(data, 1);
  size_t datasize;
  
  if(mode == MODE_RECV) {
    size_t maxsize = count * 5;
    char*  listptr = NULL;

    data->dataptr = pool_malloc(NULL, maxsize + sizeof(size_t));
    listptr = ((char*)data->dataptr) + sizeof(size_t);
    
    for(i=0; i<count; i++) {
      short itemid;
      if(net_recv(host->s, listptr, 2) != NETOK) return 1;
      itemid = ntohs(*(short*)listptr);
      listptr += 2;
      if(itemid != -1) {
        if(net_recv(host->s, listptr, 3) != NETOK) return 1;
        listptr += 3;
      }
    }
    (*(size_t*)data->dataptr) = listptr - (char*)data->dataptr - sizeof(size_t);
  }
  else {
    datasize = *(size_t*)data->dataptr;
    if(net_send(host->s, ((char*)data->dataptr) + sizeof(size_t), datasize) != NETOK)
      return PROXY_ERROR;
  }
  
  return PROXY_OK;
}

static int
helper_map_data(cid_t client_id, char mode, unsigned char msg_id, nethost_t* host, objlist_t* data, void* extra)
{
  size_t datasize = (unsigned char)proto_getc(data, 2);
  if(datasize == 0) return PROXY_OK;
  return proxy_transfer(mode, host, data, datasize);
}

int
proxy_handler_unknown(cid_t client_id, char direction, unsigned char msg_id, nethost_t* hfrom, nethost_t* hto, objlist_t* data, void* extra)
{
  log_print(NULL, "(%04d) Unsupported packet type: 0x%02x", client_id, msg_id);
  return PROXY_ERROR;
}

int 
proxy_handler_debug(cid_t client_id, char direction, unsigned char msg_id, nethost_t* hfrom, nethost_t* hto, objlist_t* data, void* extra)
{
  log_print(NULL, "(%04d) Intercepted packet: 0x%02x (%s)", client_id, msg_id,
	    direction==MSG_TOSERVER?"to server":"to client");
  return PROXY_OK;
}

int
proxy_handler_throttle(cid_t client_id, char direction, unsigned char msg_id, nethost_t* hfrom, nethost_t* hto, objlist_t* data, void *extra)
{
  static uint64_t last_connection = 0;
  unsigned long delay = *(unsigned long*)extra;

  if(direction != MSG_TOSERVER)
    return PROXY_OK;

  while(util_time() - last_connection < delay) {
    if(sys_status() == SYSTEM_SHUTDOWN)
      return PROXY_OK;
    usleep(10000);
  }
  last_connection = util_time();
  return PROXY_OK;
}

int
proxy_handler_drop(cid_t client_id, char direction, unsigned char msg_id, nethost_t* hfrom, nethost_t* hto, objlist_t* data, void* extra)
{
  int drop_rule = MSG_TOANY;
  if(extra) drop_rule = *(int*)extra;

  if(drop_rule == MSG_TOANY)
    return PROXY_NOSEND;
  if(drop_rule == direction)
    return PROXY_NOSEND;
  else
    return PROXY_OK;
}

int proxy_transfer(char mode, nethost_t* host, objlist_t* data, size_t datasize)
{
  if(mode == MODE_RECV) {
    data->dataptr = pool_malloc(NULL, datasize);
    if(net_recv(host->s, (char*)data->dataptr, datasize) != NETOK)
      return PROXY_ERROR;
  }
  else {
    if(net_send(host->s, (char*)data->dataptr, datasize) != NETOK)
      return PROXY_ERROR;
  }
  return PROXY_OK;
}

int proxy_sendmsg(unsigned char msg_id, nethost_t* host, const objlist_t* list, unsigned long flags)
{
  if(net_send(host->s, (char*)&msg_id, 1) != NETOK)
    return PROXY_ERROR;
  if(proto_send(host, list, flags) != 0)
    return PROXY_ERROR;
  return PROXY_OK;
}

msgdesc_t* proxy_init(void)
{
  size_t i, j;
  msgdesc_t* msglookup = malloc(256 * sizeof(msgdesc_t));
  memset(msglookup, 0, 256 * sizeof(msgdesc_t));
  
  j=0;
  for(i=0; i<256; i++) {
    if(msgtable[j].id == i)
      msglookup[i] = msgtable[j++];
    else {
      msglookup[i].id = i;
      msglookup[i].handler = proxy_handler_unknown;
    }
  }

  // Internal helpers
  msglookup[0x0F].datahelper = helper_generic_item;
  msglookup[0x17].datahelper = helper_add_object;
  msglookup[0x33].datahelper = helper_map_chunk;
  msglookup[0x34].datahelper = helper_multiblock_change;
  msglookup[0x3C].datahelper = helper_explosion;
  msglookup[0x66].datahelper = helper_generic_item;
  msglookup[0x67].datahelper = helper_set_slot;
  msglookup[0x68].datahelper = helper_window_items;
  msglookup[0x83].datahelper = helper_map_data;
  return msglookup;
}

void proxy_free(msgdesc_t* msglookup)
{
  free(msglookup);
}

void proxy_register(msgdesc_t* msglookup, unsigned char msgid, handler_func_t handler, void* handler_extra, helper_func_t datahelper, void* datahelper_extra)
{
  msglookup[msgid].handler = handler;
  msglookup[msgid].handler_extra = handler_extra;
  if(datahelper) {
    msglookup[msgid].datahelper = datahelper;
    msglookup[msgid].datahelper_extra = datahelper_extra;
  }
}

void proxy_event_notify(event_t* events, unsigned char type, event_func_t callback, void* callback_extra)
{
  events[type].callback = callback;
  events[type].callback_extra = callback_extra;
}
