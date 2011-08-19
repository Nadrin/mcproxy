/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_THREAD_H
#define __MCPROXY_THREAD_H

#include <stdint.h>
#include <pthread.h>

#define MCP_TLS_INITIALIZER { 0, 0 }

struct tls_value_s
{
  pthread_key_t key;
  uint8_t       init;
};
typedef struct tls_value_s tls_value_t;

void  thread_tls_initonce(tls_value_t* value);
void* thread_tls_get(tls_value_t* value);
void  thread_tls_set(tls_value_t* value, void* ptr);

void  thread_lock(void);
void  thread_unlock(void);
int   thread_trylock(void);

#endif
