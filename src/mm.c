/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdlib.h>
#include <memory.h>
#include <sys/mman.h>
#include <pthread.h>

#include <mm.h>
#include <stdio.h>

static __thread mempool_t* default_pool = NULL;
static pthread_mutex_t default_mutex    = PTHREAD_MUTEX_INITIALIZER;

mempool_t* pool_create(mempool_t* pool, size_t bytes)
{
  pool->base = mmap(NULL, bytes, PROT_READ | PROT_WRITE,
		    MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
		    -1, 0);
  
  if(pool->base == MAP_FAILED) {
    pool->base = NULL;
    return NULL;
  }
  
  pool->size  = bytes;
  pool->index = 0;
  pool->ptr   = (char*)pool->base;
  return pool;
}

void pool_release(mempool_t* pool)
{
  if(pool->base) {
    munmap(pool->base, pool->size);
    pool->base = NULL;
  }
}

inline void* pool_malloc(mempool_t* pool, size_t bytes)
{
  void* addr;
  if(!pool)
    pool = default_pool;

  addr = (void*)pool->ptr;
  pool->index += bytes;
  pool->ptr   += bytes;
  return addr;
}

inline void pool_free(mempool_t* pool)
{
  if(!pool)
    pool = default_pool;
  if(pool->index > 0)
    memset(pool->base, 0, pool->index);

  pool->index = 0;
  pool->ptr   = (char*)pool->base;
}

inline void pool_set_default(mempool_t* pool)
{
  default_pool = pool;
}

inline void thread_lock(void)
{
  pthread_mutex_lock(&default_mutex);
}

inline void thread_unlock(void)
{
  pthread_mutex_unlock(&default_mutex);
}

inline int thread_trylock(void)
{
  return pthread_mutex_trylock(&default_mutex);
}
