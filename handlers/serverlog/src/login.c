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
#include <strings.h>
#include <glib.h>

#include <mcproxy.h>
#include <gamestate.h>
#include <login.h>

void login_init_config(login_handler_config_t* cfg,
		       gamestate_t* gs,
		       const char* oplist, const char* whitelist, const char* banlist)
{
  memset(cfg, 0, sizeof(login_handler_config_t));
  strncpy(cfg->oplist, oplist, PATH_MAX);
  strncpy(cfg->whitelist, whitelist, PATH_MAX);
  strncpy(cfg->banlist, banlist, PATH_MAX);
  cfg->gs = gs;
}

void login_set_message(login_handler_config_t* cfg, int which, const char* msg)
{
  if(which == LOGIN_MSG_WHITELIST)
    strncpy(cfg->msg_whitelist, msg, 100);
  else if(which == LOGIN_MSG_BANNED)
    strncpy(cfg->msg_banlist, msg, 100);
}

void login_set_motd(login_handler_config_t* cfg, const char* filename)
{
  strncpy(cfg->motd, filename, PATH_MAX);
}

void login_set_log(login_handler_config_t* cfg, const char* filename, const char* fmt)
{
  strncpy(cfg->log, filename, PATH_MAX);
  strncpy(cfg->timefmt, fmt, 100);
}

int login_event_disconnect(cid_t client_id, unsigned char type,
			   nethost_t* client, nethost_t* server, void* extra)
{
  login_handler_config_t* config = (login_handler_config_t*)extra;
  thread_lock();
  {
    GList* found = g_list_find_custom(config->gs->playerdb,
				      &client_id, gs_find_bycid);
    if(found) {
      config->gs->playerdb = g_list_remove(config->gs->playerdb, found->data);
      gs_set_player(NULL);
    }
  }
  thread_unlock();
  return PROXY_OK;
}

int login_handler_loginrequest(cid_t client_id, char direction, unsigned char msg_id,
			       nethost_t* hfrom, nethost_t* hto, objlist_t* data,
			       void* extra)
{
  login_handler_config_t* config = (login_handler_config_t*)extra;
  char buffer[256];
  int op_login = 0;

  switch(direction) {
  case MSG_TOCLIENT:
    thread_lock();
    {
      GList* found = g_list_find_custom(config->gs->playerdb,
					&client_id, gs_find_bycid);
      if(!found) {
	thread_unlock();
	log_print(NULL, "(%04d) login_handler_loginrequest: No preliminary player data recieved!", client_id);
	return PROXY_ERROR;
      }

      gs_set_player((player_t*)found->data);
      gs_get_player()->entity_id = proto_geti(data, 0);
      gs_get_player()->dim       = abs(proto_getc(data, 4));
    }
    thread_unlock();

    log_print(NULL, "(%04d) Player logged in: %s %s (with entity ID %d)", client_id,
	      gs_get_player()->username,
	      (gs_get_player()->flags & GS_PLAYER_OP)?"<operator>":"<normal>",
	      gs_get_player()->entity_id);
    log_print(NULL, "(%04d) Player %s spawned in: %s", client_id,
	      gs_get_player()->username, gs_get_dimstr(gs_get_player()->dim));

    if(config->log[0])
      util_file_putlog(config->log, config->timefmt, gs_get_player()->username);
    if(config->motd[0])
      util_file_tochat(config->motd, hto);
    break;
  case MSG_TOSERVER:
    proto_getstr(data, 1, buffer, 256);
    op_login = util_file_find(config->oplist, buffer);

    if(op_login != 1 && util_file_find(config->banlist, buffer) == 1) {
      objlist_t* kickmsg = proto_new("t", config->msg_banlist);
      if(proxy_sendmsg(0xFF, hfrom, kickmsg, OBJFLAG_NORMAL) != PROXY_OK)
	return PROXY_ERROR;
      log_print(NULL, "(%04d) Kicking banned player: %s", client_id, buffer);
      return PROXY_DISCONNECT;
    }
    if(op_login != 1 && util_file_find(config->whitelist, buffer) == 0) {
      objlist_t* kickmsg = proto_new("t", config->msg_whitelist);
      if(proxy_sendmsg(0xFF, hfrom, kickmsg, OBJFLAG_NORMAL) != PROXY_OK)
	return PROXY_ERROR;
      log_print(NULL, "(%04d) Player not on whitelist: %s", client_id, buffer);
      return PROXY_DISCONNECT;
    }

    thread_lock();
    {
      player_t* player;
      GList* found = g_list_find_custom(config->gs->playerdb,
					buffer, gs_find_byname);
      if(found)
	config->gs->playerdb = g_list_remove(config->gs->playerdb, found->data);

      player = gs_player_init(g_malloc(sizeof(player_t)), buffer, client_id, 0);
      if(op_login == 1)
	player->flags |= GS_PLAYER_OP;
      config->gs->playerdb = g_list_append(config->gs->playerdb, player);
    }
    thread_unlock();
    break;
  }
  return PROXY_OK;
}

int login_handler_kick(cid_t client_id, char direction, unsigned char msg_id,
		       nethost_t* hfrom, nethost_t* hto, objlist_t* data,
		       void* extra)
{
  login_handler_config_t* config = (login_handler_config_t*)extra;

  thread_lock();
  {
    GList* found = g_list_find_custom(config->gs->playerdb,
				      &client_id, gs_find_bycid);
    if(!found) {
      thread_unlock();
      return PROXY_OK; // Assume list ping
    }
    config->gs->playerdb = g_list_remove(config->gs->playerdb, found->data);
  }
  thread_unlock();

  gs_set_player(NULL);
  if(direction == MSG_TOSERVER)
    log_print(NULL, "(%04d) Player logged out", client_id);
  else
    log_print(NULL, "(%04d) Player was kicked by the server", client_id);
  
  return PROXY_OK;
}
