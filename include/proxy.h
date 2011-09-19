/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_PROXY_H
#define __MCPROXY_PROXY_H

#include <network.h>
#include <proto.h>

#define MSG_TOCLIENT       0x00
#define MSG_TOSERVER       0x01
#define MSG_TOANY          0x02

#define MODE_RECV          0x00
#define MODE_SEND          0x01

#define EVENT_CONNECTED    0x00
#define EVENT_DISCONNECTED 0x01
#define EVENT_MAX          2

#define PROXY_OK           0x00
#define PROXY_NOSEND       0x01
#define PROXY_DISCONNECT   0x02
#define PROXY_ERROR        0x0A

typedef int (*event_func_t)(cid_t client_id, unsigned char type,
			    nethost_t* client, nethost_t* server,
			    void* extra);

typedef int (*handler_func_t)(cid_t client_id, char direction,
			      unsigned char msg_id,
			      nethost_t* hfrom, nethost_t* hto,
			      objlist_t* data, void* extra);

typedef int (*helper_func_t)(cid_t client_id, char mode,
			     unsigned char msg_id, nethost_t* host,
			     objlist_t* data, void* extra);

struct msgdesc_s
{
  unsigned char id;
  char* format;
  handler_func_t handler;
  helper_func_t datahelper;
  void* handler_extra;
  void* datahelper_extra;
};
typedef struct msgdesc_s msgdesc_t;

struct event_s
{
  event_func_t callback;
  void* callback_extra;
};
typedef struct event_s event_t;

msgdesc_t* proxy_init(void);
void       proxy_free(msgdesc_t* msglookup);

void proxy_register(msgdesc_t* msglookup, unsigned char msgid,
		    handler_func_t handler, void* handler_extra,
		    helper_func_t datahelper, void* datahelper_extra);
void proxy_event_notify(event_t* events, unsigned char type,
			event_func_t callback, void* callback_extra);
int  proxy_transfer(char mode, nethost_t* host, objlist_t* data, size_t datasize);
int  proxy_sendmsg(unsigned char msg_id, nethost_t* host, 
		   const objlist_t* list, unsigned long flags);

// Auxiliary handlers
int
proxy_handler_unknown(unsigned long client_id, char direction, unsigned char msg_id,
		      nethost_t* hfrom, nethost_t* hto, objlist_t* data, void* extra);
int
proxy_handler_debug(unsigned long client_id, char direction, unsigned char msg_id,
		    nethost_t* hfrom, nethost_t* hto, objlist_t* data, void* extra);
int
proxy_handler_throttle(cid_t client_id, char direction, unsigned char msg_id,
		       nethost_t* hfrom, nethost_t* hto, objlist_t* data, void *extra);
int
proxy_handler_drop(cid_t client_id, char direction, unsigned char msg_id,
		   nethost_t* hfrom, nethost_t* hto, objlist_t* data, void* extra);

#endif
