/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_UTIL_H
#define __MCPROXY_UTIL_H

#include <stdint.h>
#include <network.h>
#include <sys/types.h>

#define MCP_COLOR_BLACK       '0'
#define MCP_COLOR_DARKBLUE    '1'
#define MCP_COLOR_DARKGREEN   '2'
#define MCP_COLOR_DARKCYAN    '3'
#define MCP_COLOR_DARKRED     '4'
#define MCP_COLOR_PURPLE      '5'
#define MCP_COLOR_GOLD        '6'
#define MCP_COLOR_GRAY        '7'
#define MCP_COLOR_DARKGRAY    '8'
#define MCP_COLOR_BLUE        '9'
#define MCP_COLOR_BRIGHTGREEN 'a'
#define MCP_COLOR_CYAN        'b'
#define MCP_COLOR_RED         'c'
#define MCP_COLOR_PINK        'd'
#define MCP_COLOR_YELLOW      'e'
#define MCP_COLOR_WHITE       'f'

uint64_t util_time(void);
char*    util_time_string(char* buffer, size_t bufmax, const char* format);
char*    util_color(char* buffer, const char color);

int      util_file_writepid(const char* filename);
mode_t   util_file_stat(const char* path);
int      util_file_find(const char* filename, const char* key);
int      util_file_tochat(const char* filename, nethost_t* host);
int      util_file_putlog(const char* filename, const char* timefmt, const char* string);

size_t util_iconv_ucs2(char* dest, const size_t destsize, const char* src, const size_t srcsize);
size_t util_iconv_utf8(char* dest, const size_t destsize, const char* src, const size_t srcsize);

#endif
