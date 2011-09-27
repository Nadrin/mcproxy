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
#include <chat.h>

#define CHAT_WIDTH  50

void chat_init_config(chat_handler_config_t* cfg, gamestate_t* gs)
{
  cfg->gs = gs;
}

int chat_handler_main(cid_t client_id, char direction, unsigned char msg_id,
		      nethost_t* hfrom, nethost_t* hto, objlist_t* data,
		      void* extra)
{
  chat_handler_config_t* config = (chat_handler_config_t*)extra;
  player_t* player;

  char  message[256];
  char  text[256];
  char* args[3];
  char* saveptr;
  objlist_t* msg;

  if(proto_getstr(data, 0, message, 256) != 0)
    return PROXY_ERROR;
  if(direction == MSG_TOCLIENT)
    return PROXY_OK;
  if(message[0] != '/')
    return PROXY_OK;

  player = gs_get_player();

  thread_mutex_lock(NULL);  
  args[0] = strtok_r(message, " ", &saveptr);
  args[1] = strtok_r(NULL, " ", &saveptr);
  args[2] = strtok_r(NULL, "\n", &saveptr);

  if(strcmp(args[0], "/op") == 0) {
    if(args[1] && (player->flags & GS_PLAYER_OP)) {
      GList* found = g_list_find_custom(config->gs->playerdb, args[1], gs_find_byname);
      if(found) {
	player_t* player = (player_t*)found->data;
	player->flags |= GS_PLAYER_OP;
      }
    }
  }
  else if(strcmp(args[0], "/deop") == 0) {
    if(args[1] && (player->flags & GS_PLAYER_OP)) {
      GList* found = g_list_find_custom(config->gs->playerdb, args[1], gs_find_byname);
      if(found) {
	player_t* player = (player_t*)found->data;
	player->flags ^= GS_PLAYER_OP;
	player->flags ^= GS_PLAYER_HIDDEN;
      }
    }
  }
  else if(strcmp(args[0], "/cloak") == 0) {
    sprintf(util_color(text, MCP_COLOR_YELLOW), "Cloak engaged!");
    if((player->flags & GS_PLAYER_OP) && !(player->flags & GS_PLAYER_HIDDEN)) {
      player->flags |= GS_PLAYER_HIDDEN;
      msg = proto_new("t", text);
      if(proxy_sendmsg(0x03, hfrom, msg, OBJFLAG_NORMAL) != PROXY_OK) {
	thread_mutex_unlock(NULL);
	return PROXY_ERROR;
      }
      log_print(NULL, "(%04d) Player %s engaged the cloak",
		client_id, player->username);
    }
    thread_mutex_unlock(NULL);
    return PROXY_NOSEND;
  }
  else if(strcmp(args[0], "/decloak") == 0) {
    sprintf(util_color(text, MCP_COLOR_YELLOW), "Cloak disengaged!");
    if((player->flags & GS_PLAYER_OP) && (player->flags & GS_PLAYER_HIDDEN)) {
      player->flags ^= GS_PLAYER_HIDDEN;
      msg = proto_new("t", text);
      if(proxy_sendmsg(0x03, hfrom, msg, OBJFLAG_NORMAL) != PROXY_OK) {
	thread_mutex_unlock(NULL);
	return PROXY_ERROR;
      }
      log_print(NULL, "(%04d) Player %s disengaged the cloak",
		client_id, player->username);
    }
    thread_mutex_unlock(NULL);
    return PROXY_NOSEND;
  }
  else if(strcmp(args[0], "/tell") == 0) {
    if(args[1] && args[2]) {
      sprintf(util_color(text, MCP_COLOR_GRAY),
	      "You whisper to %s: %s", args[1], args[2]);
      msg = proto_new("t", text);
      if(proxy_sendmsg(0x03, hfrom, msg, OBJFLAG_NORMAL) != PROXY_OK) {
	thread_mutex_unlock(NULL);
	return PROXY_ERROR;
      }
    }
  }
  else if(strcmp(args[0], "/list") == 0) {
    GList* players;
    players = g_list_first(config->gs->playerdb);
    if(!players) {
      thread_mutex_unlock(NULL);
      return PROXY_ERROR;
    }

    sprintf(util_color(text, MCP_COLOR_YELLOW),
	    "Total players: %d ", g_list_length(config->gs->playerdb));
    msg = proto_new("t", text);
    if(proxy_sendmsg(0x03, hfrom, msg, OBJFLAG_NORMAL) != PROXY_OK) {
      thread_mutex_unlock(NULL);
      return PROXY_ERROR;
    }

    strcpy(text, "- ");
    do {
      player_t* player = (player_t*)players->data;
      char uname[100];
      if((player->flags & GS_PLAYER_OP))
	strcpy(util_color(uname, MCP_COLOR_RED), player->username);
      else
	strcpy(util_color(uname, MCP_COLOR_WHITE), player->username);
      
      if(strlen(text) + strlen(uname) >= CHAT_WIDTH) {
	msg = proto_new("t", text);
	if(proxy_sendmsg(0x03, hfrom, msg, OBJFLAG_NORMAL) != PROXY_OK) {
	  thread_mutex_unlock(NULL);
	  return PROXY_ERROR;
	}
	strcpy(util_color(text, MCP_COLOR_WHITE), "- ");
      }
      strcat(text, uname);
      strcat(text, " ");
    } while((players = g_list_next(players)));

    if(strlen(text) > 0) {
      msg = proto_new("t", text);
      if(proxy_sendmsg(0x03, hfrom, msg, OBJFLAG_NORMAL) != PROXY_OK) {
	thread_mutex_unlock(NULL);
	return PROXY_ERROR;
      }
    }
    thread_mutex_unlock(NULL);
    return PROXY_NOSEND;
  }

  thread_mutex_unlock(NULL);
  return PROXY_OK;
}
