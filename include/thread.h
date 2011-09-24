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

#define THREAD_DETACHED 0x01
#define THREAD_NOPOOL   0x02

#define MCP_TLS_INITIALIZER { 0, 0 }

// Opaque thread object type
typedef struct thread_object_t* thread_object_t;
typedef int (*thread_func_t)(void*);

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

thread_object_t thread_create(thread_func_t entrypoint, void* data,
			      unsigned long flags, size_t pool_size);
thread_object_t thread_self(void);

int  thread_join(thread_object_t thread);
void thread_sync(unsigned short count);

#endif
