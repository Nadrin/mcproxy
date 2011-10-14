/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak, Dylan Lukes
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include <util.h>

#include <config.h>
#include <proxy.h>

#include <iconv.h>
#include <errno.h>

uint64_t util_time(void)
{
  struct timeval tv;
  if(gettimeofday(&tv, NULL) != 0)
    return 0;
  return tv.tv_sec*1000 + tv.tv_usec/1000;
}

char* util_time_string(char* buffer, size_t bufmax, const char* format)
{
  struct tm timedata;
  time_t now = time(NULL);
  localtime_r(&now, &timedata);
  strftime(buffer, bufmax, format, &timedata);
  return buffer;
}

char* util_color(char* buffer, const char color)
{
  buffer[0] = 0xC2;
  buffer[1] = 0xA7;
  buffer[2] = color;
  return buffer + 3;
}

int util_file_writepid(const char* filename)
{
  FILE* file = fopen(filename, "w");
  if(!file) return 1;
  fprintf(file, "%lu\n", (long unsigned int)getpid());
  fclose(file);
  return 0;
}

mode_t util_file_stat(const char* path)
{
  struct stat statbuf;
  if(stat(path, &statbuf) == -1)
    return 0;
  return statbuf.st_mode;
}

int util_file_find(const char* filename, const char* key)
{
  int found  = 0;
  char buffer[256];
  FILE* file;

  file = fopen(filename, "r");
  if(!file) return -1;

  while(fscanf(file, "%s", buffer) == 1) {
    if(strncasecmp(buffer, key, 256) == 0) {
      found = 1;
      break;
    }
  }
  
  fclose(file);
  return found;
}

int util_file_tochat(const char* filename, nethost_t* host)
{
  FILE* file;
  char buffer[256];
  
  file = fopen(filename, "r");
  if(!file) return -1;

  while(fgets(buffer, 256, file)) {
    objlist_t* chatmsg;
    size_t msglen = strlen(buffer);

    if(msglen < 2) 
      continue;
    if(buffer[msglen-1] == '\n')
      buffer[msglen-1] = 0;

    chatmsg = proto_new("t", buffer);
    if(proxy_sendmsg(0x03, host, chatmsg, OBJFLAG_NORMAL) != PROXY_OK) {
      fclose(file);
      return -1;
    }
  }
  fclose(file);
  return 0;
}

int util_file_putlog(const char* filename, const char* timefmt, const char* string)
{
  char buffer[256];
  FILE* file;

  file = fopen(filename, "a");
  if(!file) return -1;

  util_time_string(buffer, 256, timefmt);
  fprintf(file, "%s %s\n", buffer, string);
  fclose(file);
  return 0;
}

static size_t
util_iconv_generic(iconv_t context, char* dest, const size_t destsize, const char* src, const size_t srcsize)
{
  char* _dest = (char*)dest;
  char* _src  = (char*)src;
  size_t src_bytes = srcsize;
  size_t dst_bytes = destsize;
  
  memset(dest, 0, destsize);
  while(src_bytes > 0) {
    if(iconv(context, &_src, &src_bytes, &_dest, &dst_bytes) == (size_t)(-1)) {
      if(errno == E2BIG)
        return src_bytes;
      else
        return (size_t)(-1);
    }
  }
  return 0;
]

size_t util_iconv_ucs2(char* dest, const size_t destsize, const char* src, const size_t srcsize)
{
  iconv_t context;
  size_t retvalue;
  
  context  = iconv_open("UCS-2BE", "UTF-8");
  retvalue = util_iconv_generic(context, dest, destsize, src, srcsize);
  iconv_close(context);
  return retvalue;
}

size_t util_iconv_utf8(char* dest, const size_t destsize, const char* src, const size_t srcsize)
{
  iconv_t context;
  size_t retvalue;

  context  = iconv_open("UTF-8", "UCS-2BE");
  retvalue = util_iconv_generic(context, dest, destsize, src, srcsize);
  iconv_close(context);
  return retvalue;
}
