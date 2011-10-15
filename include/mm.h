/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak, Dylan Lukes
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_MM_H
#define __MCPROXY_MM_H

struct mempool_s
{
  void* base;
  char* ptr;
  size_t index;
  size_t size;
};
typedef struct mempool_s mempool_t;

mempool_t* pool_create(mempool_t* pool, size_t bytes);
void       pool_release(mempool_t* pool);
void*      pool_malloc(mempool_t* pool, size_t bytes);
void       pool_free(mempool_t* pool);
void       pool_set_default(mempool_t* pool);

#endif
