/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
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
#include <unistd.h>

#include <config.h>
#include <log.h>
#include <network.h>
#include <proto.h>
#include <proxy.h>
#include <core.h>
#include <util.h>
#include <mm.h>

// Global quit flag
extern volatile sig_atomic_t mcp_quit;

int core_dispatch(thread_data_t* data, char direction,
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
  if(handler_result != PROXY_NOSEND) {
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

static void* core_thread(void* data)
{
  thread_data_t* thread_data = (thread_data_t*)data;
  nethost_t *server = NULL, *client = NULL;
  event_t* ev;
  mempool_t pool;
  char errormsg[256];
  int net_error = CORE_ENETWORK;

  pthread_detach(pthread_self());

  client = net_accept(thread_data->listen_sockfd);
  thread_data->flags |= THREAD_FLAG_RUNNING;
  if(!client)
    goto _thread_done;

  server = net_connect(thread_data->server_addr, thread_data->server_port);
  if(!server)
    goto _thread_done;

  if(!pool_create(&pool, MCPROXY_THREAD_POOL)) {
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
  log_print(NULL, "(%04d) Client connected from %s on port %d",
	    thread_data->client_id, client->addr, client->port);

  net_error = errno = 0;
  while(mcp_quit == 0) {
    unsigned long sockready = net_select_rd(2, client->s, server->s);
    if(sockready == 0) {
      net_error = CORE_ENETWORK;
      break;
    }

    pool_free(&pool);
    if(sockready & 0x01) { // client to server
      net_error = core_dispatch(thread_data, MSG_TOSERVER, client, server);
      if(net_error != 0) break;
    }
    if(sockready & 0x02) { // server to client
      net_error = core_dispatch(thread_data, MSG_TOCLIENT, server, client);
      if(net_error != 0) break;
    }
    errno = 0;
  }
  pool_free(&pool);

 _thread_done:
  ev = &thread_data->events[EVENT_DISCONNECTED];
  if(ev->callback) {
    if(ev->callback(thread_data->client_id, EVENT_DISCONNECTED, client, server,
		    ev->callback_extra) != PROXY_OK)
      log_print(NULL, "(%04d) DISCONNECTED event callback returned error status",
		thread_data->client_id);
  }
  
  if(net_error == CORE_EOK || net_error == CORE_EDONE) {
    usleep(100000);
    log_print(NULL, "(%04d) Client disconnected", thread_data->client_id);
  }
  else {
    if(errno == 0) strcpy(errormsg, "Client was disconnected by server");
    else strerror_r(errno, errormsg, 256);
    log_print(NULL, "(%04d) Network error: %s", thread_data->client_id, errormsg);
  }
  
 _thread_exit:
  if(server) {
    net_close(server->s);
    free(server);
  }
  if(client) {
    net_close(client->s);
    free(client);
  }

  pool_release(&pool);
  while(thread_data->flags & THREAD_FLAG_CREATE)
    usleep(1000);
  free(thread_data);
  pthread_exit(NULL);
}

int core_throttle(uint64_t* last, unsigned long delay)
{
  while(util_time() - *last < delay) {
    if(mcp_quit != 0) return mcp_quit;
    usleep(10000);
  }
  *last = util_time();
  return mcp_quit;
}

int core_main(const char* server_addr, int server_port, int listen_port, int debug,
	      handler_api_t* handler_api)
{
  unsigned long client_id = 0;
  msgdesc_t*    msgtable  = NULL;
  event_t       events[EVENT_MAX];

  int      listen_sockfd;
  sigset_t blocked_signals;
  uint64_t last_connection = 0;

  pthread_attr_t thread_attr;
  pthread_attr_init(&thread_attr);
  pthread_attr_setstacksize(&thread_attr, MCPROXY_THREAD_STACK);

  sigemptyset(&blocked_signals);
  sigaddset(&blocked_signals, SIGPIPE);
  sigaddset(&blocked_signals, SIGHUP);
  pthread_sigmask(SIG_BLOCK, &blocked_signals, NULL);

  listen_sockfd = net_listen(listen_port);
  if(listen_sockfd <= 0) {
    log_print(NULL, "Cannot bind to port %d! Aborting.", listen_port);
    return EXIT_FAILURE;
  }

  msgtable = proxy_init();
  memset(events, 0, EVENT_MAX*sizeof(event_t));
  log_print(NULL, "Minecraft Proxy started, listening on port %d", listen_port);

  if(handler_api->handler_startup(msgtable, events, debug) != PROXY_OK) {
    log_print(NULL, "Handler initialization failed! Aborting.");
    net_close(listen_sockfd);
    proxy_free(msgtable);
    return EXIT_FAILURE;
  }
  
  log_print(NULL, "Handler library initialized: %s", handler_api->handler_info());
  while(mcp_quit == 0) {
    pthread_t thread_id;
    thread_data_t* thread_data = NULL;

    if(!(net_select_rd(1, listen_sockfd) & 0x01))
      continue;
    if(core_throttle(&last_connection, MCPROXY_THROTTLE) != 0)
      break;
    
    thread_data = malloc(sizeof(thread_data_t));
    thread_data->client_id     = ++client_id;
    thread_data->listen_sockfd = listen_sockfd;
    thread_data->lookup        = msgtable;
    thread_data->events        = events;

    thread_data->server_addr = server_addr;
    thread_data->server_port = server_port;
    thread_data->flags       = THREAD_FLAG_CREATE;

    if(debug & LOG_DEBUG)
      thread_data->flags |= THREAD_FLAG_DEBUG;

    memset(&thread_id, 0, sizeof(pthread_t));
    if(pthread_create(&thread_id, &thread_attr, core_thread, (void*)thread_data) != 0)
      break;
    while(!(thread_data->flags & THREAD_FLAG_RUNNING))
      usleep(1000);
    thread_data->flags ^= THREAD_FLAG_CREATE;
  }

  net_close(listen_sockfd);
  usleep(100000); // Wait for threads to catch mcp_quit

  handler_api->handler_shutdown();
  proxy_free(msgtable);
  log_print(NULL, "Shutdown completed");
  return EXIT_SUCCESS;
}
