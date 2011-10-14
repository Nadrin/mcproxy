/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdlib.h>
#include <signal.h>
#include <dlfcn.h>
#include <memory.h>
#include <unistd.h>

#include <config.h>
#include <system.h>

static unsigned int  _sys_mode = MCP_MODE_UNSPEC;
static char**        _sys_argv = NULL;
static int           _sys_argc = 0;

// This is the actual mcp_quit, all other references are extern
volatile sig_atomic_t mcp_quit = 0;

int sys_initapi(void* library, handler_api_t* handler_api)
{
  // dlsym returns a void*. Casting from an object pointer to a function pointer is dissallowed by the C99 standard.
  // We can do this in a platform independent way by passing through a union.
  union {void *optr; void (*fptr)(void); } optr_to_fptr;

  optr_to_fptr.optr = dlsym(library, "handler_info");
  handler_api->handler_info     = (handler_info_func_t) optr_to_fptr.fptr;
  optr_to_fptr.optr = dlsym(library, "handler_startup");
  handler_api->handler_startup  = (handler_startup_func_t) optr_to_fptr.fptr;
  optr_to_fptr.optr = dlsym(library, "handler_shutdown");
  handler_api->handler_shutdown = (handler_shutdown_func_t) optr_to_fptr.fptr;
  
  if(!handler_api->handler_info ||
     !handler_api->handler_startup ||
     !handler_api->handler_shutdown)
    return SYSTEM_ERROR;
  return SYSTEM_OK;
}

sys_config_t* sys_get_config(void)
{
  static sys_config_t config;
  static int config_init = 0;

  if(!config_init) {
    config.pool_size = MCPROXY_THREAD_POOL;
    config_init = 1;
  }
  return &config;
}

unsigned int sys_get_mode(void)
{
  return _sys_mode;
}

const char* sys_get_modestring(void)
{
  switch(_sys_mode) {
  case MCP_MODE_CLIENT: return "client";
  case MCP_MODE_SERVER: return "server";
  case MCP_MODE_PROXY:  return "proxy";
  default: return "unspecified";
  }
}

int sys_set_mode(unsigned int mode)
{
  if(_sys_mode != MCP_MODE_UNSPEC)
    return SYSTEM_ERROR;
  if(mode == MCP_MODE_UNSPEC || mode > MCP_MODE_MAX)
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

int sys_status(void)
{
  if(mcp_quit != 0)
    return SYSTEM_SHUTDOWN;
  return SYSTEM_OK;
}
