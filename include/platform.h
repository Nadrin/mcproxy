/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak, Dylan Lukes
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_PLATFORM_H
#define __MCPROXY_PLATFORM_H

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define be16toh(x) OSSwapBigToHostInt16(x)
#define htobe16(x) OSSwapHostToBigInt16(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define htobe32(x) OSSwapHostToBigInt32(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define htobe64(x) OSSwapHostToBigInt64(x)

#ifndef MCPROXY_USE_NAMED_SEMAPHORES
#define MCPROXY_USE_NAMED_SEMAPHORES 1
#endif

#else
#include <endian.h>
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#endif
