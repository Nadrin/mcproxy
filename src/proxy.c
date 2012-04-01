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
extern short eidtable[];

static int
helper_add_object(cid_t client_id, char mode, unsigned char msg_id,
                  nethost_t* host, objlist_t* data, void* extra)
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
helper_map_chunk(cid_t client_id, char mode, unsigned char msg_id,
                 nethost_t* host, objlist_t* data, void* extra)
{
  size_t datasize = proto_geti(data, 5);
  return proxy_transfer(mode, host, data, datasize);
}

static int
helper_multiblock_change(cid_t client_id, char mode, unsigned char msg_id,
                         nethost_t* host, objlist_t* data, void* extra)
{
  size_t datasize = proto_geti(data, 3);
  return proxy_transfer(mode, host, data, datasize);
}

static int
helper_plugin_message(cid_t client_id, char mode, unsigned char msg_id,
                      nethost_t* host, objlist_t* data, void* extra)
{
  size_t datasize = proto_gets(data, 1);
  return proxy_transfer(mode, host, data, datasize);
}

static int
helper_explosion(cid_t client_id, char mode, unsigned char msg_id,
                 nethost_t* host, objlist_t* data, void* extra)
{
  size_t datasize = proto_geti(data, 4) * 3;
  if(datasize == 0) return PROXY_OK;
  return proxy_transfer(mode, host, data, datasize);
}

static int
helper_map_data(cid_t client_id, char mode, unsigned char msg_id,
                nethost_t* host, objlist_t* data, void* extra)
{
  size_t datasize = (unsigned char)proto_getc(data, 2);
  if(datasize == 0) return PROXY_OK;
  return proxy_transfer(mode, host, data, datasize);
}

static int
helper_slot(cid_t client_id, char mode, unsigned char msg_id,
            nethost_t* host, objlist_t* data, void* extra)
{
  objlist_t* slot;
  slot_t slotdata;
  
  size_t index = (size_t)extra;

  if(mode == MODE_RECV) {
    slot = pool_malloc(NULL, sizeof(objlist_t));
    
    slot->objects = pool_malloc(NULL, 4 * sizeof(object_t));
    slot->count   = 4;
    
    if(proto_recv_slot(host, 0, slot) != 0)
      return PROXY_ERROR;
    data->objects[index].data = (void*)slot;
  }
  else {
    slot = (objlist_t*)data->objects[index].data;
    if(proto_send_slot(host, 0, slot) != 0)
      return PROXY_ERROR;
  }

  proto_getslot(slot, 0, &slotdata);
  if(slotdata.datasize > 0)
    return proxy_transfer(mode, host, data, slotdata.datasize);
  return PROXY_OK;
}

static int
helper_slot_array(cid_t client_id, char mode, unsigned char msg_id,
                  nethost_t* host, objlist_t* data, void* extra)
{
  objlist_t *array;
  slot_t slotdata;
  int errcode;
  
  size_t i, index = (size_t)extra;
  size_t count    = (size_t)proto_gets(data, index-1);

  if(mode == MODE_RECV) {
    array = pool_malloc(NULL, count * sizeof(objlist_t));
    for(i=0; i<count; i++) {
      array[i].objects = pool_malloc(NULL, 4 * sizeof(object_t));
      array[i].count   = 4;

      if(proto_recv_slot(host, 0, &array[i]) != 0)
        return PROXY_ERROR;

      proto_getslot(&array[i], 0, &slotdata);
      if(slotdata.datasize > 0) {
        errcode = proxy_transfer(mode, host, &array[i], slotdata.datasize);
        if(errcode != PROXY_OK) return errcode;
      }
    }
    data->objects[index].data = (void*)array;
  }
  else {
    array = (objlist_t*)data->objects[index].data;
    for(i=0; i<count; i++) {
      if(proto_send_slot(host, 0, &array[i]) != 0)
        return PROXY_ERROR;

      proto_getslot(&array[i], 0, &slotdata);
      if(slotdata.datasize > 0) {
        errcode = proxy_transfer(mode, host, &array[i], slotdata.datasize);
        if(errcode != PROXY_OK) return errcode;
      }
    }
  }

  return PROXY_OK;
}

int
proxy_handler_unknown(cid_t client_id, char direction, unsigned char msg_id,
                      nethost_t* hfrom, nethost_t* hto, objlist_t* data, void* extra)
{
  log_print(NULL, "(%04d) Unsupported packet type: 0x%02x", client_id, msg_id);
  return PROXY_ERROR;
}

int 
proxy_handler_debug(cid_t client_id, char direction, unsigned char msg_id,
                    nethost_t* hfrom, nethost_t* hto, objlist_t* data, void* extra)
{
  log_print(NULL, "(%04d) Intercepted packet: 0x%02x (%s)", client_id, msg_id,
	    direction==MSG_TOSERVER?"to server":"to client");
  return PROXY_OK;
}

int
proxy_handler_throttle(cid_t client_id, char direction, unsigned char msg_id,
                       nethost_t* hfrom, nethost_t* hto, objlist_t* data, void *extra)
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
proxy_handler_drop(cid_t client_id, char direction, unsigned char msg_id,
                   nethost_t* hfrom, nethost_t* hto, objlist_t* data, void* extra)
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

int proxy_sendmsg(unsigned char msg_id, nethost_t* host,
                  const objlist_t* list, unsigned long flags)
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
  msglookup[0x17].datahelper = helper_add_object;
  msglookup[0x33].datahelper = helper_map_chunk;
  msglookup[0x34].datahelper = helper_multiblock_change;
  msglookup[0x3C].datahelper = helper_explosion;

  msglookup[0x0F].datahelper = helper_slot;
  msglookup[0x0F].datahelper_extra = (void*)4;
  msglookup[0x66].datahelper = helper_slot;
  msglookup[0x66].datahelper_extra = (void*)5;
  msglookup[0x67].datahelper = helper_slot;
  msglookup[0x67].datahelper_extra = (void*)2;
  msglookup[0x6B].datahelper = helper_slot;
  msglookup[0x6B].datahelper_extra = (void*)1;
  
  msglookup[0x68].datahelper = helper_slot_array;
  msglookup[0x68].datahelper_extra = (void*)2;
  
  msglookup[0x83].datahelper = helper_map_data;
  msglookup[0xFA].datahelper = helper_plugin_message;
  return msglookup;
}

void proxy_free(msgdesc_t* msglookup)
{
  free(msglookup);
}

void proxy_register(msgdesc_t* msglookup, unsigned char msgid,
                    handler_func_t handler, void* handler_extra,
                    helper_func_t datahelper, void* datahelper_extra)
{
  msglookup[msgid].handler = handler;
  msglookup[msgid].handler_extra = handler_extra;
  if(datahelper) {
    msglookup[msgid].datahelper = datahelper;
    msglookup[msgid].datahelper_extra = datahelper_extra;
  }
}

void proxy_event_notify(event_t* events, unsigned char type,
                        event_func_t callback, void* callback_extra)
{
  events[type].callback = callback;
  events[type].callback_extra = callback_extra;
}
