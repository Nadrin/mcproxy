/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <config.h>
#include <proto.h>
#include <util.h>
#include <mm.h>

// Floating point <-> integer conversion unions
union _float32_conv
{
  float    fvalue;
  uint32_t ivalue;
};
union _float64_conv
{
  double   fvalue;
  uint64_t ivalue;
};

unsigned char proto_typeof(objlist_t* list, size_t index)
{
  if(index >= list->count)
    return TYPE_INVALID;
  return list->objects[index].type;
}

inline char proto_getc(objlist_t* list, size_t index)
{ 
  return *(char*)list->objects[index].data;
}

inline void proto_putc(objlist_t* list, size_t index, char value)
{ 
  (*(char*)list->objects[index].data) = value;
}

inline short proto_gets(objlist_t* list, size_t index)
{ 
  return be16toh(*(short*)list->objects[index].data);
}

inline void proto_puts(objlist_t* list, size_t index, short value)
{
  (*(short*)list->objects[index].data) = htobe16(value);
}

inline int32_t proto_geti(objlist_t* list, size_t index)
{
  return be32toh(*(int32_t*)list->objects[index].data);
}

inline void proto_puti(objlist_t* list, size_t index, int32_t value)
{
  (*(int32_t*)list->objects[index].data) = htobe32(value);
}

inline int64_t proto_getl(objlist_t* list, size_t index)
{
  return be64toh(*(int64_t*)list->objects[index].data);
}

inline void proto_putl(objlist_t* list, size_t index, int64_t value)
{
  (*(int64_t*)list->objects[index].data) = htobe64(value);
}

inline float proto_getf(objlist_t* list, size_t index)
{
  union _float32_conv conv;
  conv.ivalue = be32toh(*(uint32_t*)list->objects[index].data);
  return conv.fvalue;
}

inline void proto_putf(objlist_t* list, size_t index, float value)
{
  union _float32_conv conv;
  conv.fvalue = value;
  *((uint32_t*)list->objects[index].data) = htobe32(conv.ivalue);
}

inline double proto_getd(objlist_t* list, size_t index)
{
  union _float64_conv conv;
  conv.ivalue = be64toh(*(uint64_t*)list->objects[index].data);
  return conv.fvalue;
}

inline void proto_putd(objlist_t* list, size_t index, double value)
{
  union _float64_conv conv;
  conv.fvalue = value;
  *((uint64_t*)list->objects[index].data) = htobe64(conv.ivalue);
}

size_t proto_getstr(objlist_t* list, size_t index, char* buffer, size_t maxsize)
{
  char* bytes = ((char*)(list->objects[index].data)) + 2;
  size_t size = list->objects[index].size - 2;

  return util_iconv_utf8(buffer, maxsize, bytes, size);
}

size_t proto_getustr(objlist_t* list, size_t index, char* buffer, size_t maxsize)
{
  char* bytes = (char*)list->objects[index].data;
  size_t size = list->objects[index].size - 2;
  
  if(size >= maxsize)
    return size - maxsize + 1;
  
  if(size > 0)
    memcpy(buffer, bytes + 2, size);
  buffer[size] = 0;
  return 0;
}

void proto_putstr(objlist_t* list, size_t index, const char* value)
{
  size_t len     = strlen(value);
  size_t bufsize = 2 + (len << 1);
  char*  bytes   = pool_malloc(NULL, bufsize);

  *((short*)bytes) = htons(len);
  util_iconv_ucs2(bytes + 2, bufsize - 2, value, len);

  list->objects[index].data = (void*)bytes;
  list->objects[index].size = bufsize;
}

void proto_putustr(objlist_t* list, size_t index, const char *value)
{
  size_t len  = strlen(value);
  char* bytes = pool_malloc(NULL, 2 + len);

  (*(short*)bytes) = htons(len);
  if(len > 0)
    memcpy(bytes + 2, value, len);
  list->objects[index].data = (void*)bytes;
  list->objects[index].size = len + 2;
}

int proto_send_meta(nethost_t* host, const object_t* curobj)
{
  objlist_t* metalist = (objlist_t*)curobj->data;
  return proto_send(host, metalist, OBJFLAG_NORMAL);
}

int proto_send_object(nethost_t* host, const object_t* curobj)
{
  if(curobj->type == TYPE_INVALID)
    return 0;
  if(curobj->type == TYPE_META)
    return proto_send_meta(host, curobj);
  if(net_send(host->s, (char*)curobj->data, curobj->size) != NETOK)
    return 1;
  return 0;
}

int proto_send(nethost_t* host, const objlist_t* list, unsigned long flags)
{
  size_t i;
  for(i=0; i<list->count; i++) {
    object_t* curobj = &list->objects[i];
    if(curobj->flags != flags)
      continue;
    if(proto_send_object(host, curobj) != 0)
      return 1;
  }
  return 0;
}

int proto_recv_meta(nethost_t* host, object_t* curobj)
{
  objlist_t* metalist;
  size_t i   = 0;
  int retval = 1;

  static char types[] = "csift";

  metalist = pool_malloc(NULL, sizeof(objlist_t));

  metalist->count   = MCPROXY_META_MAX;
  metalist->objects = pool_malloc(NULL, MCPROXY_META_MAX * sizeof(object_t));

  while(proto_recv_object(host, &metalist->objects[i++], 'c') == 0) {
    char gvalue = *(char*)metalist->objects[i-1].data;
    if(gvalue == 127)
      break;

    if((gvalue >> 5) <= sizeof(types)-1)
      retval = proto_recv_object(host, &metalist->objects[i++], types[gvalue >> 5]);
    else {
      retval =  proto_recv_object(host, &metalist->objects[i++], 's');
      retval += proto_recv_object(host, &metalist->objects[i++], 'c');
      retval += proto_recv_object(host, &metalist->objects[i++], 's');
    }
    if(retval != 0)
      return retval;
  }
  
  metalist->count = i;
  curobj->data = (void*)metalist;
  return 0;
}

void proto_object_init(object_t* object, unsigned char type)
{
  static size_t _sizes[] = { 0, 1, 2, 4, 8, 4, 8, 0, 0, 0, 0 };

  if(type == TYPE_BLANK)
    object->flags |= OBJFLAG_HANDLER;

  object->size = _sizes[type];
  object->type = type;
  if(object->size > 0)
    object->data = pool_malloc(NULL, object->size);
}

int proto_recv_object(nethost_t* host, object_t* curobj, const char format)
{
  short slen, host_slen;
  char buf[8];

  switch(format) {
  case '-': // Blank
    proto_object_init(curobj, TYPE_BLANK);
    break;
  case 'c': // Byte, bool
    if(net_recv(host->s, buf, 1) != NETOK) return 2;
    proto_object_init(curobj, TYPE_BYTE);
    memcpy(curobj->data, buf, 1);
    break;
  case 's': // Short
    if(net_recv(host->s, buf, 2) != NETOK) return 2;
    proto_object_init(curobj, TYPE_SHORT);
    memcpy(curobj->data, buf, 2);
    break;
  case 'i': // Int
    if(net_recv(host->s, buf, 4) != NETOK) return 2;
    proto_object_init(curobj, TYPE_INT);
    memcpy(curobj->data, buf, 4);
    break;
  case 'l': // Long
    if(net_recv(host->s, buf, 8) != NETOK) return 2;
    proto_object_init(curobj, TYPE_LONG);
    memcpy(curobj->data, buf, 8);
    break;
  case 'f': // Float
    if(net_recv(host->s, buf, 4) != NETOK) return 2;
    proto_object_init(curobj, TYPE_FLOAT);
    memcpy(curobj->data, buf, 4);
    break;
  case 'd': // Double
    if(net_recv(host->s, buf, 8) != NETOK) return 2;
    proto_object_init(curobj, TYPE_DOUBLE);
    memcpy(curobj->data, buf, 8);
    break;
  case 't': // String
  case 'u': // UTF8 String
    if(net_recv(host->s, (char*)&slen, 2) != NETOK) return 2;
    host_slen = ntohs(slen);
    
    if(format == 't') {
      host_slen <<= 1;
      curobj->type = TYPE_STRING;
    }
    else
      curobj->type = TYPE_USTRING;

    curobj->size = 2 + host_slen;
    curobj->data = pool_malloc(NULL, curobj->size);
    memcpy(curobj->data, &slen, 2);
    
    if(host_slen > 0) {
      if(net_recv(host->s, ((char*)curobj->data) + 2, host_slen) != NETOK)
	return 2;
    }
    break;
  case 'm': // Metadata
    proto_object_init(curobj, TYPE_META);
    return proto_recv_meta(host, curobj);
  default:
    return 1;
  }
  return 0;
}

objlist_t* proto_recv(nethost_t* host, const char* format)
{
  size_t i;
  
  objlist_t* list = pool_malloc(NULL, sizeof(objlist_t));
  list->count     = strlen(format);
  list->objects   = pool_malloc(NULL, list->count*sizeof(object_t));
  list->dataptr   = NULL;

  for(i=0; i<list->count; i++) {
    object_t* curobj = &list->objects[i];
    if(proto_recv_object(host, curobj, format[i]) != 0)
      return NULL;
  }
  return list;
}

objlist_t* proto_new(const char* format, ...)
{
  size_t i;
  va_list ap;

  objlist_t* list = pool_malloc(NULL, sizeof(objlist_t));
  list->count     = strlen(format);
  list->objects   = pool_malloc(NULL, list->count*sizeof(object_t));
  list->dataptr   = NULL;

  va_start(ap, format);
  for(i=0; i<list->count; i++) {
    object_t* curobj = &list->objects[i];
    switch(format[i]) {
    case '-':
      proto_object_init(curobj, TYPE_BLANK);
      break;
    case 'm':
      proto_object_init(curobj, TYPE_META);
      break;
    case 'c':
      proto_object_init(curobj, TYPE_BYTE);
      proto_putc(list, i, va_arg(ap, int));
      break;
    case 's':
      proto_object_init(curobj, TYPE_SHORT);
      proto_puts(list, i, va_arg(ap, int));
      break;
    case 'i':
      proto_object_init(curobj, TYPE_INT);
      proto_puti(list, i, va_arg(ap, int));
      break;
    case 'l':
      proto_object_init(curobj, TYPE_LONG);
      proto_putl(list, i, va_arg(ap, long));
      break;
    case 'f':
      proto_object_init(curobj, TYPE_FLOAT);
      proto_putf(list, i, va_arg(ap, double));
      break;
    case 'd':
      proto_object_init(curobj, TYPE_DOUBLE);
      proto_putd(list, i, va_arg(ap, double));
      break;
    case 't':
      proto_object_init(curobj, TYPE_STRING);
      proto_putstr(list, i, va_arg(ap, char*));
      break;
    case 'u':
      proto_object_init(curobj, TYPE_USTRING);
      proto_putustr(list, i, va_arg(ap, char*));
      break;
    default:
      va_end(ap);
      return NULL;
    }
  }
  va_end(ap);
  return list;
}
