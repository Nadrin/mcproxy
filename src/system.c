/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdlib.h>
#include <dlfcn.h>

#include <config.h>
#include <system.h>

static unsigned int _sys_mode = MODE_TYPE_UNSPEC;
static char**       _sys_argv = NULL;
static int          _sys_argc = 0;

int sys_init(void* library, handler_api_t* handler_api)
{
  handler_api->handler_info     = dlsym(library, "handler_info");
  handler_api->handler_startup  = dlsym(library, "handler_startup");
  handler_api->handler_shutdown = dlsym(library, "handler_shutdown");
  
  if(!handler_api->handler_info ||
     !handler_api->handler_startup ||
     !handler_api->handler_shutdown)
    return SYSTEM_ERROR;
  return SYSTEM_OK;
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
  if(_sys_mode != MODE_TYPE_UNSPEC)
    return SYSTEM_ERROR;
  if(mode == MODE_TYPE_UNSPEC || mode >= MODE_TYPE_MAX)
    return SYSTEM_ERROR;
  _sys_mode = mode;
  return SYSTEM_OK;
}

void sys_set_args(int argc, char** argv)
{
  _sys_argc = argc;
  _sys_argv = argv;
}

int sys_argc(void)
{
  return _sys_argc;
}

char** sys_argv(void)
{
  return _sys_argv;
}
