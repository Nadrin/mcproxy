/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdlib.h>

#include <config.h>
#include <thread.h>
#include <system.h>
#include <mm.h>

static pthread_mutex_t   default_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t   barrier_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned short    barrier_count = 0;
static pthread_barrier_t default_barrier;

struct entrypoint_info_s
{
  thread_func_t user_func;
  void* user_data;
  unsigned long flags;
  size_t pool_size;
};
typedef struct entrypoint_info_s entrypoint_info_t;

static void* _thread_entrypoint(void* data)
{
  entrypoint_info_t* entrypoint_info = (entrypoint_info_t*)data;
  mempool_t thread_pool;
  int retvalue;

  if(entrypoint_info->flags & THREAD_DETACHED)
    pthread_detach(pthread_self());

  if(!(entrypoint_info->flags & THREAD_NOPOOL)) {
    size_t pool_size = entrypoint_info->pool_size;
    if(pool_size == 0)
      pool_size = sys_get_config()->pool_size;
    if(pool_create(&thread_pool, pool_size))
      pool_set_default(&thread_pool);
  }

  retvalue = entrypoint_info->user_func(entrypoint_info->user_data);

  if(!(entrypoint_info->flags & THREAD_NOPOOL))
    pool_release(&thread_pool);

  free(entrypoint_info);
  pthread_exit((void*)(size_t)retvalue);
}

void thread_tls_initonce(tls_value_t* value)
{
  if(__sync_bool_compare_and_swap(&value->init, 0, 1))
    pthread_key_create(&value->key, NULL);
}

void* thread_tls_get(tls_value_t* value)
{
  return pthread_getspecific(value->key);
}

void thread_tls_set(tls_value_t* value, void* ptr)
{
  pthread_setspecific(value->key, ptr);
}

void thread_lock(void)
{
  pthread_mutex_lock(&default_mutex);
}

void thread_unlock(void)
{
  pthread_mutex_unlock(&default_mutex);
}

int thread_trylock(void)
{
  return pthread_mutex_trylock(&default_mutex);
}

thread_object_t thread_create(thread_func_t entrypoint, void* data,
			      unsigned long flags, size_t pool_size)
{
  pthread_t thread_id;
  entrypoint_info_t* info = malloc(sizeof(entrypoint_info_t));
  info->user_func = entrypoint;
  info->user_data = data;
  info->flags     = flags;
  info->pool_size = pool_size;

  if(pthread_create(&thread_id, NULL, _thread_entrypoint, (void*)info) != 0) {
    free(info);
    return NULL;
  }
  return (thread_object_t)thread_id;
}

int thread_join(thread_object_t thread)
{
  void* return_data;
  if(pthread_join((pthread_t)thread, &return_data) != 0)
    return -1;
  return (int)(size_t)return_data;
}

void thread_sync(unsigned short count)
{
  pthread_mutex_lock(&barrier_mutex);
  if(barrier_count != count) {
    pthread_barrier_init(&default_barrier, NULL, count);
    barrier_count = count;
  }
  pthread_mutex_unlock(&barrier_mutex);

  pthread_barrier_wait(&default_barrier);

  pthread_mutex_lock(&barrier_mutex);
  if(barrier_count > 0) {
    pthread_barrier_destroy(&default_barrier);
    barrier_count = 0;
  }
  pthread_mutex_unlock(&barrier_mutex);
}

thread_object_t thread_self(void)
{
  return (thread_object_t)pthread_self();
}
