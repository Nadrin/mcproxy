/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdlib.h>
#include <thread.h>

static pthread_mutex_t default_mutex = PTHREAD_MUTEX_INITIALIZER;

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

