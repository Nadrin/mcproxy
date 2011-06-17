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

#include <glib.h>
#include <mcproxy.h>

#include <gamestate.h>
#include <login.h>
#include <action.h>
#include <inventory.h>

#include <settings.h>

#define KEYFILE_GROUP "configuration"

#define KEYFILE_VALUE(array, index) \
  (array[(3*index)+2]?array[(3*index)+2]:array[(3*index)+1])

int settings_read_config(const char* filename,
			 login_handler_config_t* config_login,
			 gamestate_t* gamestate,
			 char* item_list, char* action_list)
{
  static const gchar* search_dirs[] = { ".", "mcproxy", "bin/mcproxy", NULL };
  GKeyFile* keyfile = g_key_file_new();
  GError* error;

  int i;
  gchar* value;

  gchar* config[] = {
    "operators", "ops.txt", NULL,
    "white-list", "white-list.txt", NULL,
    "banned-list", "banned-players.txt", NULL,
    "logfile", "logins.log", NULL,
    "motd", "motd.txt", NULL,
    "message-whitelist", "You are not on the whitelist!", NULL,
    "message-banned", "You are banned on this server!", NULL,
    "trackfile-items", "tracked-items.txt", NULL,
    "trackfile-actions", "tracked-actions.txt", NULL,
    "date-format", "%Y-%m-%d %H:%M:%S", NULL,
    NULL,
  };
  
  if(!g_key_file_load_from_dirs(keyfile, filename, search_dirs, NULL,
				G_KEY_FILE_NONE, &error)) {
    log_print(NULL, "Could not read %s: %s", filename, error->message);
    g_error_free(error);
    g_key_file_free(keyfile);
    return 1;
  }

  if(!g_key_file_has_group(keyfile, KEYFILE_GROUP)) {
    log_print(NULL, "Configuration group [%s] could not be found", KEYFILE_GROUP);
    g_key_file_free(keyfile);
    return 2;
  }

  i=0;
  while(config[i]) {
    value = g_key_file_get_string(keyfile, KEYFILE_GROUP, config[i], NULL);
    if(value)
      config[i+2] = strcpy(malloc(strlen(value)+1), value);
    i+=3;
  }
  g_key_file_free(keyfile);

  login_init_config(config_login, gamestate,
		    KEYFILE_VALUE(config, 0),
		    KEYFILE_VALUE(config, 1),
		    KEYFILE_VALUE(config, 2));

  login_set_log(config_login, KEYFILE_VALUE(config, 3), KEYFILE_VALUE(config, 9));
  login_set_motd(config_login, KEYFILE_VALUE(config, 4));
  login_set_message(config_login, LOGIN_MSG_WHITELIST, KEYFILE_VALUE(config, 5));
  login_set_message(config_login, LOGIN_MSG_BANNED, KEYFILE_VALUE(config, 6));

  if(item_list)
    strcpy(item_list, KEYFILE_VALUE(config, 7));
  if(action_list)
    strcpy(action_list, KEYFILE_VALUE(config, 8));

  i=0;
  while(config[i]) {
    if(config[i+2])
      free(config[i+2]);
    i+=3;
  }
  return 0;
}

int settings_read_list(const char* filename, 
		       void* userdata,
		       settings_list_callback_t callback)
{
  char  buffer[100];
  char  desc[100];
  FILE* file;
  int   value;

  file = fopen(filename, "r");
  if(!file)
    return 1;

  while(!feof(file)) {
    fgets(buffer, 100, file);
    if(sscanf(buffer, "%d %[^\n]", &value, desc) != 2)
      continue;
    callback(userdata, value, desc);
  }
  fclose(file);
  return 0;
}

void settings_callback_inventory(void* userdata, int value, const char* name)
{
  inventory_handler_config_t* config = (inventory_handler_config_t*)userdata;
  inventory_set_watch(config, value, name);
}

void settings_callback_actions(void* userdata, int value, const char* name)
{
  action_handler_config_t* config = (action_handler_config_t*)userdata;
  action_set_watch(config, value, name);
}
