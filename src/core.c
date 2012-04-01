/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak, Dylan Lukes
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/utsname.h>

#include <config.h>
#include <system.h>
#include <log.h>
#include <network.h>
#include <proto.h>
#include <proxy.h>
#include <core.h>
#include <util.h>
#include <mm.h>
#include <thread.h>

static int core_dispatch(thread_data_t* data, char direction,
                         nethost_t* host_from, nethost_t* host_to)
{
  unsigned char msgid;
  msgdesc_t* msgdesc = NULL;
  objlist_t* objlist = NULL;
  int handler_result;
  
  // Recv data
  if(net_recv(host_from->s, (char*)&msgid, 1) != NETOK)
    return CORE_ENETWORK;
  msgdesc = &data->lookup[msgid];

#ifdef ENABLE_DEBUG
  {
    static int debug_msgseen[256];
    if(debug_msgseen[msgid] == 0) {
      fprintf(stderr, "[%s] 0x%02x\n", direction==MSG_TOSERVER?"C->S":"S->C", msgid);
      debug_msgseen[msgid] = 1;
    }
  }
#endif

  if(msgdesc->format) {
    objlist = proto_recv(host_from, msgdesc->format);
    if(!objlist)
      return CORE_ENETWORK;
  }

  // Datahelper (from -> to)
  if(msgdesc->datahelper) {
    if(msgdesc->datahelper(data->client_id, MODE_RECV,
                           msgid, host_from,
                           objlist, msgdesc->datahelper_extra) != PROXY_OK)
      return CORE_EDATAHELPER;
  }
  
  // Handler
  handler_result = PROXY_OK;
  if(msgdesc->handler) {
    handler_result = msgdesc->handler(data->client_id, direction,
                                      msgid, host_from, host_to,
                                      objlist, msgdesc->handler_extra);
    if(handler_result == PROXY_ERROR)
      return CORE_EHANDLER;
  }
  
  // Send data
  if(host_to && handler_result != PROXY_NOSEND) {
    if(net_send(host_to->s, (char*)&msgid, 1) != NETOK)
      return CORE_ENETWORK;
    
    if(msgdesc->format) {
      if(proto_send(host_to, objlist, OBJFLAG_NORMAL) != 0)
        return CORE_ENETWORK;
    }
    
    // Datahelper (to -> from)
    if(msgdesc->datahelper) {
      if(msgdesc->datahelper(data->client_id, MODE_SEND,
                             msgid, host_to,
                             objlist, msgdesc->datahelper_extra) != PROXY_OK)
        return CORE_EDATAHELPER;
    }
  }

  if(handler_result == PROXY_DISCONNECT)
    return CORE_EDONE;
  return (msgid != 0xFF)?CORE_EOK:CORE_EDONE;
}

static int core_loop_client(nethost_t* server, thread_data_t* thread_data)
{
  if(net_select_rd(1, server->s) == 0)
    return CORE_ENETWORK;

  return core_dispatch(thread_data, MSG_TOCLIENT, server, NULL);
}

static int core_loop_server(nethost_t* client, thread_data_t* thread_data)
{
  if(net_select_rd(1, client->s) == 0)
    return CORE_ENETWORK;

  return core_dispatch(thread_data, MSG_TOSERVER, client, NULL);
}

static int core_loop_proxy(nethost_t* client, nethost_t* server, thread_data_t* thread_data)
{
  int net_error;
  unsigned long sockready = net_select_rd(2, client->s, server->s);
  if(sockready == 0)
    return CORE_ENETWORK;
  
  if(sockready & 0x01) { // client to server
    net_error = core_dispatch(thread_data, MSG_TOSERVER, client, server);
    if(net_error != CORE_EOK) return net_error;
  }
  if(sockready & 0x02) { // server to client
    net_error = core_dispatch(thread_data, MSG_TOCLIENT, server, client);
    if(net_error != CORE_EOK) return net_error;
  }
  return CORE_EOK;
}

static void* core_thread(void* data)
{
  thread_data_t* thread_data = (thread_data_t*)data;
  nethost_t *server = NULL, *client = NULL;
  event_t* ev;
  mempool_t pool;
  char errormsg[256];
  int net_error = CORE_EOK;

  pthread_detach(pthread_self());
  thread_data->flags ^= THREAD_FLAG_CREATE;

  CORE_MODE(MCP_MODE_SERVER | MCP_MODE_PROXY) {
    client = net_accept(thread_data->listen_sockfd);
    thread_barrier_wait((thread_barrier_t*)thread_data->sync_primitive);
    if(!client)
      goto _thread_done;
  }

  CORE_MODE(MCP_MODE_CLIENT | MCP_MODE_PROXY) {
    server = net_connect(thread_data->server_addr, thread_data->server_port, NULL);
    if(!server)
      goto _thread_done;
  }

  if(!pool_create(&pool, sys_get_config()->pool_size)) {
    log_print(NULL, "(%04d) Thread memory pool allocation failed",
              thread_data->client_id);
    goto _thread_exit;
  }
  pool_set_default(&pool);

  ev = &thread_data->events[EVENT_CONNECTED];
  if(ev->callback) {
    if(ev->callback(thread_data->client_id, EVENT_CONNECTED, client, server,
                    ev->callback_extra) != PROXY_OK) {
      log_print(NULL, "(%04d) CONNECTED event callback returned error status",
                thread_data->client_id);
      goto _thread_exit;
    }
  }

  CORE_MODE(MCP_MODE_CLIENT) {
    log_print(NULL, "(%04d) Connected to server at %s on port %d",
              thread_data->client_id, thread_data->server_addr, thread_data->server_port);
  }
  CORE_MODE(MCP_MODE_SERVER | MCP_MODE_PROXY) {
    log_print(NULL, "(%04d) Client connected from %s on port %d",
              thread_data->client_id, client->addr, client->port);
  }

  thread_data->flags |= THREAD_FLAG_RUNNING;
  while(sys_status() == SYSTEM_OK && net_error == CORE_EOK) {
    errno = 0;
    pool_free(&pool);

    CORE_MODE(MCP_MODE_CLIENT) {
      net_error = core_loop_client(server, thread_data);
    }
    CORE_MODE(MCP_MODE_SERVER) {
      net_error = core_loop_server(client, thread_data);
    }
    CORE_MODE(MCP_MODE_PROXY) {
      net_error = core_loop_proxy(client, server, thread_data);
    }
  }
  pool_free(&pool);

 _thread_done:
  ev = &thread_data->events[EVENT_DISCONNECTED];
  if(ev->callback) {
    if(ev->callback(thread_data->client_id, EVENT_DISCONNECTED, client, server,
                    ev->callback_extra) != PROXY_OK)
      log_print(NULL, "(%04d) DISCONNECTED event callback returned error status", thread_data->client_id);
  }
  
  if(net_error == CORE_EOK || net_error == CORE_EDONE) {
    usleep(100000);
    log_print(NULL, "(%04d) Disconnected", thread_data->client_id);
  }
  else {
    if(errno == 0) strcpy(errormsg, "Peer disconnected unexpectedly");
    else strerror_r(errno, errormsg, 256);
    log_print(NULL, "(%04d) Network error: %s", thread_data->client_id, errormsg);
  }
  
 _thread_exit:
  if(server) net_free(server);
  if(client) net_free(client);
  pool_release(&pool);

  CORE_MODE(MCP_MODE_CLIENT) {
    thread_barrier_wait((thread_barrier_t*)thread_data->sync_primitive);
  }
  free(thread_data);
  pthread_exit(NULL);
}

static int core_throttle(uint64_t* last, unsigned long delay)
{
  int status;
  while(util_time() - *last < delay) {
    if((status = sys_status()) != SYSTEM_OK)
      return status;
    usleep(10000);
  }
  *last = util_time();
  return sys_status();
}

int core_main(sys_config_t* system_config, handler_api_t* handler_api)
{
  unsigned long   client_id    = 0;
  msgdesc_t*      msgtable     = NULL;
  handler_info_t* handler_info = NULL;
  event_t         events[EVENT_MAX];

  sigset_t blocked_signals;
  int      listen_sockfd   = 0;
  uint64_t last_connection = 0;

  thread_barrier_t sync_barrier;
  pthread_attr_t   thread_attr;

  struct utsname   platform_info;
  uname(&platform_info);
  log_print(NULL, "Minecraft Proxy, version %s starting ...", MCPROXY_VERSION_STR);
  log_print(NULL, "%s %s at %s (%s)",
            platform_info.sysname, platform_info.release,
            platform_info.nodename, platform_info.machine);

  thread_barrier_init(&sync_barrier, 2);
  pthread_attr_init(&thread_attr);
  pthread_attr_setstacksize(&thread_attr, MCPROXY_THREAD_STACK);

  sigemptyset(&blocked_signals);
  sigaddset(&blocked_signals, SIGPIPE);
  sigaddset(&blocked_signals, SIGHUP);
  pthread_sigmask(SIG_BLOCK, &blocked_signals, NULL);

  handler_info = handler_api->handler_info();
  if(sys_set_mode(handler_info->type) != SYSTEM_OK) {
    log_print(NULL, "Failed to initialize requested mode of operation: 0x%02x", handler_info->type);
    return EXIT_FAILURE;
  }
  log_print(NULL, "Requested mode of operation: 0x%02x", handler_info->type);

  CORE_MODE(MCP_MODE_SERVER | MCP_MODE_PROXY) {
    listen_sockfd = net_listen(system_config->listen_port, NULL);
    if(listen_sockfd <= 0) {
      log_print(NULL, "Cannot bind to port %s! Aborting.", system_config->listen_port);
      return EXIT_FAILURE;
    }
    log_print(NULL, "Listening on port %s", system_config->listen_port);
  }

  msgtable = proxy_init();
  memset(events, 0, EVENT_MAX*sizeof(event_t));
  log_print(NULL, "Core initialized in %s mode", sys_get_modestring());
  
  if(handler_api->handler_startup(msgtable, events) != SYSTEM_OK) {
    log_print(NULL, "Handler initialization failed! Aborting.");
    net_close(listen_sockfd);
    proxy_free(msgtable);
    return EXIT_FAILURE;
  }
  log_print(NULL, "Handler library loaded: %s, version: %d",
            handler_info->name, handler_info->version);

  log_print(NULL, "Startup completed, entering main loop");
  while(sys_status() == SYSTEM_OK) {
    pthread_t         thread_id;
    thread_data_t*    thread_data = NULL;

    CORE_MODE(MCP_MODE_SERVER | MCP_MODE_PROXY) {
      if(!(net_select_rd(1, listen_sockfd) & 0x01))
        continue;
    }
    if(core_throttle(&last_connection, system_config->connect_delay) != SYSTEM_OK)
      break;

    thread_data = malloc(sizeof(thread_data_t));
    thread_data->client_id     = ++client_id;
    thread_data->listen_sockfd = listen_sockfd;
    thread_data->lookup        = msgtable;
    thread_data->events        = events;

    thread_data->server_addr = system_config->server_addr;
    thread_data->server_port = system_config->server_port;
    thread_data->flags       = THREAD_FLAG_CREATE;

    if(system_config->debug & LOG_DEBUG)
      thread_data->flags |= THREAD_FLAG_DEBUG;

    thread_data->sync_primitive = (void*)&sync_barrier;
    memset(&thread_id, 0, sizeof(pthread_t));
    if(pthread_create(&thread_id, &thread_attr, core_thread, (void*)thread_data) != 0)
      break;
    thread_barrier_wait(&sync_barrier);

    CORE_MODE(MCP_MODE_CLIENT) {
      if(!system_config->connect_retry) break;
    }
  }
  log_print(NULL, "Got status change, initiating shutdown ...");

  CORE_MODE(MCP_MODE_SERVER | MCP_MODE_PROXY) {
    net_close(listen_sockfd);
  }

  usleep(100000); // Wait for threads to catch mcp_quit

  handler_api->handler_shutdown();
  proxy_free(msgtable);
  thread_barrier_free(&sync_barrier);
  log_print(NULL, "Shutdown completed");
  return EXIT_SUCCESS;
}
