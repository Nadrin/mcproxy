/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <log.h>

static log_t* log_default = NULL;

log_t* log_open(const char* filename, unsigned long flags)
{
  log_t* logfile;

  if(filename) {
    FILE* f = fopen(filename, "a");
    if(!f) return NULL;
    fclose(f);
  }

  logfile = malloc(sizeof(log_t));
  if(filename) strcpy(logfile->filename, filename);
  else logfile->filename[0] = 0;
  logfile->flags = flags;
  return logfile;
}

void log_set_default(log_t* logfile)
{
  log_default = logfile;
}

void log_print(log_t* logfile, const char* format, ...)
{
  char buffer[2048];
  char timestr[100];
  va_list ap;
  time_t now;
  struct tm timedata;

  log_t* _logfile = logfile;
  if(!_logfile) _logfile = log_default;
  if(!_logfile) return;
  
  va_start(ap, format);
  vsprintf(buffer, format, ap);
  va_end(ap);
  
  now = time(NULL);
  localtime_r(&now, &timedata);
  strftime(timestr, 100, "%Y-%m-%d %H:%M:%S", &timedata);

  if(_logfile->filename[0]) {
    FILE* f = fopen(_logfile->filename, "a");
    fprintf(f, "[%s] %s\n", timestr, buffer);
    fclose(f);
  }
  if((_logfile->flags & LOG_DEBUG) || !_logfile->filename[0])
    fprintf(stderr, "[%s] %s\n", timestr, buffer);
}

void log_close(log_t* logfile)
{
  free(logfile);
}
