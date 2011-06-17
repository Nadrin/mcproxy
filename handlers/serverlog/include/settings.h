/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 * ServerLog handler library.
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_HANDLER_SETTINGS_H
#define __MCPROXY_HANDLER_SETTINGS_H

typedef void (*settings_list_callback_t)(void*, int, const char*);

int settings_read_config(const char* filename, 
			 login_handler_config_t* config_login,
			 gamestate_t* gamestate,
			 char* item_list, char* action_list);

int settings_read_list(const char* filename, 
		       void* userdata,
		       settings_list_callback_t callback);

void settings_callback_inventory(void* userdata, int value, const char* name);
void settings_callback_actions(void* userdata, int value, const char* name);

#endif
