/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak, Dylan Lukes
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <config.h>
#include <mm.h>

static pthread_key_t   default_pool_key;
static pthread_once_t  default_pool_init = PTHREAD_ONCE_INIT;

static void pool_global_init(void)
{
  pthread_key_create(&default_pool_key, NULL);
}

mempool_t* pool_create(mempool_t* pool, size_t bytes)
{
  pthread_once(&default_pool_init, pool_global_init);
  pool->base = malloc(bytes);
    
  if(pool->base == NULL) {
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
    free(pool->base);
    pool->base = NULL;
  }
}

inline void* pool_malloc(mempool_t* pool, size_t bytes)
{
  void* addr;
  if(!pool)
    pool = pthread_getspecific(default_pool_key);

  addr = (void*)pool->ptr;
  pool->index += bytes;
  pool->ptr   += bytes;
  return addr;
}

inline void pool_free(mempool_t* pool)
{
  if(!pool)
    pool = pthread_getspecific(default_pool_key);
  if(pool->index > 0)
    memset(pool->base, 0, pool->index);

  pool->index = 0;
  pool->ptr   = (char*)pool->base;
}

inline void pool_set_default(mempool_t* pool)
{
  pthread_setspecific(default_pool_key, pool);
}
