/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_PROTO_H
#define __MCPROXY_PROTO_H

#include <stdint.h>
#include <network.h>

#define TYPE_INVALID  0
#define TYPE_BYTE     1
#define TYPE_SHORT    2
#define TYPE_INT      3
#define TYPE_LONG     4
#define TYPE_FLOAT    5
#define TYPE_DOUBLE   6
#define TYPE_STRING   7
#define TYPE_USTRING  8
#define TYPE_META     9
#define TYPE_BLANK   10

#define OBJFLAG_NORMAL  0x00
#define OBJFLAG_HANDLER 0x01

struct object_s {
  void* data;
  size_t size;
  unsigned char type;
  unsigned long flags;
};
typedef struct object_s object_t;

struct objlist_s {
  object_t* objects;
  void* dataptr;
  size_t count;
};
typedef struct objlist_s objlist_t;

void proto_object_init(object_t* object, unsigned char type);
unsigned char proto_typeof(objlist_t* list, size_t index);

inline char proto_getc(objlist_t* list, size_t index);
inline void proto_putc(objlist_t* list, size_t index, char value);
inline short proto_gets(objlist_t* list, size_t index);
inline void proto_puts(objlist_t* list, size_t index, short value);
inline int32_t proto_geti(objlist_t* list, size_t index);
inline void proto_puti(objlist_t* list, size_t index, int32_t value);
inline int64_t proto_getl(objlist_t* list, size_t index);
inline void proto_putl(objlist_t* list, size_t index, int64_t value);
inline float proto_getf(objlist_t* list, size_t index);
inline void proto_putf(objlist_t* list, size_t index, float value);
inline double proto_getd(objlist_t* list, size_t index);
inline void proto_putd(objlist_t* list, size_t index, double value);
size_t proto_getstr(objlist_t* list, size_t index, char* buffer, size_t maxsize);
void proto_putstr(objlist_t* list, size_t index, const char *value);
size_t proto_getustr(objlist_t* list, size_t index, char* buffer, size_t maxsize);
void proto_putustr(objlist_t* list, size_t index, const char *value);

int proto_send(nethost_t* host, const objlist_t* list, unsigned long flags);
objlist_t* proto_recv(nethost_t* host, const char* format);
objlist_t* proto_new(const char* format, ...);

int proto_send_object(nethost_t* host, const object_t* curobj);
int proto_send_meta(nethost_t* host, const object_t* curobj);
int proto_recv_object(nethost_t* host, object_t* curobj, const char format);
int proto_recv_meta(nethost_t* host, object_t* curobj);



#endif
