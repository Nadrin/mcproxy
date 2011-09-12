/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <dlfcn.h>

#include <config.h>
#include <system.h>

static unsigned int _sys_mode = MODE_TYPE_UNSPEC;

int sys_api_init(void* library, handler_api_t* handler_api)
{
  handler_api->handler_info     = dlsym(library, "handler_info");
  handler_api->handler_startup  = dlsym(library, "handler_startup");
  handler_api->handler_shutdown = dlsym(library, "handler_shutdown");
  
  if(!handler_api->handler_info ||
     !handler_api->handler_startup ||
     !handler_api->handler_shutdown)
    return 1;
  return 0;
}

sys_config_t* sys_get_config(void)
{
  static sys_config_t config;
  return &config;
}

unsigned int sys_get_mode(void)
{
  return _sys_mode;
}

int sys_set_mode(unsigned int mode)
{
  if(mode >= MODE_TYPE_MAX)
    return 1;
  _sys_mode = mode;
  return 0;
}
