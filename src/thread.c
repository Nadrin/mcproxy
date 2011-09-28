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

static pthread_mutex_t  default_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t  barrier_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned short   barrier_count = 0;
static thread_barrier_t default_barrier;

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

void thread_mutex_init(thread_mutex_t* mutex)
{
  pthread_mutex_init(mutex, NULL);
}

void thread_mutex_lock(thread_mutex_t* mutex)
{
  if(mutex)
    pthread_mutex_lock(mutex);
  else
    pthread_mutex_lock(&default_mutex);
}

void thread_mutex_unlock(thread_mutex_t* mutex)
{
  if(mutex)
    pthread_mutex_unlock(mutex);
  else
    pthread_mutex_unlock(&default_mutex);
}

int thread_mutex_trylock(thread_mutex_t* mutex)
{
  if(mutex)
    return pthread_mutex_trylock(mutex);
  else
    return pthread_mutex_trylock(&default_mutex);
}

int thread_barrier_init(thread_barrier_t* barrier, unsigned short nthreads)
{
  if(nthreads == 0)
    return SYSTEM_INVALID;

#ifdef __APPLE__
  static unsigned int __semcount = 0;
  sprintf(barrier->name[0], "mcproxy-%d-%d.semaphore", getpid(), __sync_add_and_fetch(&__semcount, 1));
  sprintf(barrier->name[1], "mcproxy-%d-%d.semaphore", getpid(), __sync_add_and_fetch(&__semcount, 1));
  barrier->semaphore[0] = sem_open(barrier->name[0], O_CREAT, 0, 0); 
  barrier->semaphore[1] = sem_open(barrier->name[1], O_CREAT, 0, 0);

  if(barrier->semaphore[0] == SEM_FAILED || barrier->semaphore[1] == SEM_FAILED)
    return SYSTEM_ERROR;
#else
  barrier->semaphore[0] = malloc(sizeof(sem_t));
  barrier->semaphore[1] = malloc(sizeof(sem_t));
  sem_init(barrier->semaphore[0], 0, 0);
  sem_init(barrier->semaphore[1], 0, 0);
#endif

  pthread_mutex_init(&barrier->mutex, NULL);
  barrier->count    = 0;
  barrier->nthreads = nthreads;
  return SYSTEM_OK;
}

void thread_barrier_free(thread_barrier_t* barrier)
{
#ifdef __APPLE__
  sem_close(barrier->semaphore[0]);
  sem_close(barrier->semaphore[1]);
  sem_unlink(barrier->name[0]);
  sem_unlink(barrier->name[1]);
#else
  sem_destroy(barrier->semaphore[0]);
  sem_destroy(barrier->semaphore[1]);
  free(barrier->semaphore[0]);
  free(barrier->semaphore[1]);
#endif
  barrier->nthreads = 0;
}

void thread_barrier_wait(thread_barrier_t* barrier)
{
  unsigned short i;
  if(barrier->nthreads == 0)
    return;

  pthread_mutex_lock(&barrier->mutex);
  if(++barrier->count == barrier->nthreads) {
    for(i=0; i<barrier->nthreads; i++)
      sem_post(barrier->semaphore[0]);
  }
  pthread_mutex_unlock(&barrier->mutex);
  sem_wait(barrier->semaphore[0]);

  pthread_mutex_lock(&barrier->mutex);
  if(--barrier->count == 0) {
    for(i=0; i<barrier->nthreads; i++)
      sem_post(barrier->semaphore[1]);
  }
  pthread_mutex_unlock(&barrier->mutex);
  sem_wait(barrier->semaphore[1]);
}

int thread_create(thread_object_t* thread, 
		  thread_func_t entrypoint, void* data,
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
    return SYSTEM_ERROR;
  }

  if(thread)
    *thread = (thread_object_t)thread_id;
  return SYSTEM_OK;
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
    thread_barrier_init(&default_barrier, count);
    barrier_count = count;
  }
  pthread_mutex_unlock(&barrier_mutex);

  thread_barrier_wait(&default_barrier);

  pthread_mutex_lock(&barrier_mutex);
  if(barrier_count > 0) {
    thread_barrier_free(&default_barrier);
    barrier_count = 0;
  }
  pthread_mutex_unlock(&barrier_mutex);
}

thread_object_t thread_self(void)
{
  return (thread_object_t)pthread_self();
}
