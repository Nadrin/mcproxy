/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak, Dylan Lukes
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_LOG_H
#define __MCPROXY_LOG_H

#define LOG_NOFLAGS 0x00
#define LOG_DEBUG   0x01

struct log_s {
  char filename[256];
  unsigned long flags;
};
typedef struct log_s log_t;

log_t* log_open(const char* filename, unsigned long flags);
void log_print(log_t* logfile, const char* format, ...);
void log_close(log_t* logfile);
void log_set_default(log_t* logfile);

#endif
