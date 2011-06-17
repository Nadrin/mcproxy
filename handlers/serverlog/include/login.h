/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 * ServerLog handler library.
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_HANDLERS_LOGIN_H
#define __MCPROXY_HANDLERS_LOGIN_H

#define LOGIN_MSG_WHITELIST 0x00
#define LOGIN_MSG_BANNED    0x01

struct login_handler_config_s {
  char whitelist[PATH_MAX];
  char banlist[PATH_MAX];
  char oplist[PATH_MAX];
  char motd[PATH_MAX];
  char log[PATH_MAX];
  char timefmt[100];
  char msg_whitelist[100];
  char msg_banlist[100];
  gamestate_t* gs;
};
typedef struct login_handler_config_s login_handler_config_t;

void login_init_config(login_handler_config_t* cfg, gamestate_t* gs,
		       const char* oplist, const char* whitelist, const char* banlist);
void login_set_message(login_handler_config_t* cfg, int which, const char* msg);
void login_set_motd(login_handler_config_t* cfg, const char* filename);
void login_set_log(login_handler_config_t* cfg, const char* filename, const char* fmt);

int login_event_disconnect(cid_t client_id, unsigned char type,
			   nethost_t* client, nethost_t* server, void* extra);

int login_handler_loginrequest(cid_t client_id, char direction, unsigned char msg_id,
			       nethost_t* hfrom, nethost_t* hto, objlist_t* data,
			       void* extra);

int login_handler_kick(cid_t client_id, char direction, unsigned char msg_id,
		       nethost_t* hfrom, nethost_t* hto, objlist_t* data,
		       void* extra);

#endif
