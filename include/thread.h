/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak, Dylan Lukes
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_THREAD_H
#define __MCPROXY_THREAD_H

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

#define THREAD_DETACHED 0x01
#define THREAD_NOPOOL   0x02

#define MCP_TLS_INITIALIZER { 0, 0 }

typedef pthread_t       thread_object_t;
typedef pthread_mutex_t thread_mutex_t;

typedef int (*thread_func_t)(void*);

struct tls_value_s
{
  pthread_key_t key;
  uint8_t       init;
};
typedef struct tls_value_s tls_value_t;

struct thread_barrier_s
{
  pthread_mutex_t mutex;
  sem_t* semaphore[2];
  unsigned short count;
  unsigned short nthreads;
#ifdef MCPROXY_USE_NAMED_SEMAPHORES
  char name[2][100];
#endif
};
typedef struct thread_barrier_s thread_barrier_t;

void  thread_tls_initonce(tls_value_t* value);
void* thread_tls_get(tls_value_t* value);
void  thread_tls_set(tls_value_t* value, void* ptr);

void  thread_mutex_init(thread_mutex_t* mutex);
void  thread_mutex_lock(thread_mutex_t* mutex);
void  thread_mutex_unlock(thread_mutex_t* mutex);
int   thread_mutex_trylock(thread_mutex_t* mutex);

int   thread_barrier_init(thread_barrier_t* barrier, unsigned short nthreads);
void  thread_barrier_wait(thread_barrier_t* barrier);
void  thread_barrier_free(thread_barrier_t* barrier);

int thread_create(thread_object_t* thread,  thread_func_t entrypoint, void* data, unsigned long flags, size_t pool_size);

thread_object_t thread_self(void);

int  thread_join(thread_object_t thread);
void thread_sync(unsigned short nthreads);

#endif
