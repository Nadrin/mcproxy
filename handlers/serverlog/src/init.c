/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 * ServerLog handler library.
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdlib.h>
#include <memory.h>
#include <glib.h>
#include <mcproxy.h>

#include <gamestate.h>
#include <login.h>
#include <track.h>
#include <transaction.h>
#include <inventory.h>
#include <action.h>
#include <chat.h>

#include <settings.h>

static login_handler_config_t  config_login;
static track_handler_config_t  config_track;
static action_handler_config_t config_action;
static inventory_handler_config_t config_inventory;
static chat_handler_config_t config_chat;
static gamestate_t gamestate;

char* handler_info(void)
{
  return "ServerLog v1.1";
}

int handler_startup(msgdesc_t* msglookup, event_t* events, unsigned long flags)
{
  char item_list[PATH_MAX];
  char action_list[PATH_MAX];

  // Global gamestate
  memset(&gamestate, 0, sizeof(gamestate_t));

  // Login management handlers
  if(settings_read_config("serverlog.ini", &config_login, &gamestate,
			  item_list, action_list) != 0)
    return PROXY_ERROR;

  proxy_event_notify(events, EVENT_DISCONNECTED, login_event_disconnect, &config_login);
  proxy_register(msglookup, 0x01, login_handler_loginrequest, &config_login, NULL, NULL);
  proxy_register(msglookup, 0xFF, login_handler_kick, &config_login, NULL, NULL);

  // Transaction handling
  proxy_register(msglookup, 0x6A, transact_handler_main, NULL, NULL, NULL);

  // Player tracking
  track_init_config(&config_track, &gamestate);

  proxy_register(msglookup, 0x09, track_handler_main, &config_track, NULL, NULL);
  proxy_register(msglookup, 0x0A, track_handler_main, &config_track, NULL, NULL);
  proxy_register(msglookup, 0x0B, track_handler_main, &config_track, NULL, NULL);
  proxy_register(msglookup, 0x0C, track_handler_main, &config_track, NULL, NULL);
  proxy_register(msglookup, 0x0D, track_handler_main, &config_track, NULL, NULL);
  proxy_register(msglookup, 0x14, track_handler_entity, &config_track, NULL, NULL);
  
  // Inventory tracking
  inventory_init_config(&config_inventory);
  settings_read_list(item_list, &config_inventory,
		     settings_callback_inventory);

  proxy_register(msglookup, 0x64, inventory_handler_main, &config_inventory, NULL, NULL);
  proxy_register(msglookup, 0x65, inventory_handler_main, &config_inventory, NULL, NULL);
  proxy_register(msglookup, 0x66, inventory_handler_main, &config_inventory, NULL, NULL);
  proxy_register(msglookup, 0x67, inventory_handler_main, &config_inventory, NULL, NULL);
  proxy_register(msglookup, 0x68, inventory_handler_windowitems, &config_inventory, NULL, NULL);

  // Player actions
  action_init_config(&config_action);
  settings_read_list(action_list, &config_action,
		     settings_callback_actions);

  proxy_register(msglookup, 0x07, action_handler_itemuse, &config_action, NULL, NULL);
  proxy_register(msglookup, 0x0F, action_handler_itemuse, &config_action, NULL, NULL);
  proxy_register(msglookup, 0x10, action_handler_itemuse, &config_action, NULL, NULL);
  proxy_register(msglookup, 0x3C, action_handler_explosion, &config_action, NULL, NULL);

  // Chat interception
  chat_init_config(&config_chat, &gamestate);
  proxy_register(msglookup, 0x03, chat_handler_main, &config_chat, NULL, NULL);
  return PROXY_OK;
}

int handler_shutdown(void)
{
  action_free_config(&config_action);
  inventory_free_config(&config_inventory);
  g_list_free(gamestate.playerdb);
  return PROXY_OK;
}
