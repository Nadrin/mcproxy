/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <config.h>
#include <log.h>
#include <core.h>
#include <util.h>

// Global quit flag and signal handler
volatile sig_atomic_t mcp_quit = 0;
static void sig_quit(int sig)
{
  mcp_quit = 1;
}

static void sig_daemonize(int sig)
{
  switch(sig) {
  case SIGALRM: exit(EXIT_FAILURE);
  case SIGCHLD: exit(EXIT_FAILURE);
  case SIGUSR1: exit(EXIT_SUCCESS);
  }
}

static void mcp_usage(const char* progname)
{
  fprintf(stderr, "Minecraft Proxy, Copyright (c) 2011 Michał Siejak\n");
  fprintf(stderr, "usage: %s [-d] [-p port] [-l logfile] [-r pidfile] <-L handlerlib> <server_addr> [server_port]\n", progname);
  exit(EXIT_FAILURE);
}

void mcp_parse_arguments(int argc, char **argv,
			 char* listen_port, char* server_port, char* server_addr,
			 char* libfile, char* pidfile, char* logfile, int* debug)
{
  int c;
  char  workdir[PATH_MAX];
  char* homedir = getenv("HOME");

  strcpy(listen_port, MCPROXY_DEFAULT_PORT);
  strcpy(server_port, MCPROXY_DEFAULT_PORT);

  *debug = LOG_NOFLAGS;

  getcwd(workdir, PATH_MAX);
  sprintf(pidfile, "%s/%s", homedir?homedir:"/tmp", MCPROXY_PIDFILE);
  sprintf(logfile, "%s/%s", homedir?homedir:"/tmp", MCPROXY_LOGFILE);
  libfile[0] = 0;

  while((c = getopt(argc, argv, "dp:l:r:L:")) != -1) {
    switch(c) {
    case 'd':
      *debug = LOG_DEBUG;
      break;
    case 'p':
      strcpy(listen_port, optarg);
      break;
    case 'l':
      if(optarg[0] == '/')
	strcpy(logfile, optarg);
      else
	sprintf(logfile, "%s/%s", workdir, optarg);
      break;
    case 'r':
      if(optarg[0] == '/')
	strcpy(pidfile, optarg);
      else
	sprintf(pidfile, "%s/%s", workdir, optarg);
      break;
    case 'L':
      if(optarg[0] == '/')
	strcpy(libfile, optarg);
      else
	sprintf(libfile, "%s/%s", workdir, optarg);
      break;
    default:
      mcp_usage(argv[0]);
    }
  }
  if(optind >= argc || libfile[0] == 0)
    mcp_usage(argv[0]);

  strcpy(server_addr, argv[optind]);
  if(optind+1 < argc)
    strcpy(server_port, argv[optind+1]);
 
  if(!homedir)
    fprintf(stderr, "%s: Warning, no HOME environment variable defined!\n", argv[0]);
}

void mcp_daemonize(void)
{
  pid_t pid, parent;
  
  signal(SIGCHLD, sig_daemonize);
  signal(SIGUSR1, sig_daemonize);
  signal(SIGALRM, sig_daemonize);

  pid = fork();
  if(pid < 0) 
    exit(EXIT_FAILURE);
  if(pid > 0) {
    alarm(2);
    pause();
    exit(EXIT_FAILURE);
  }

  parent = getppid();
  signal(SIGCHLD, SIG_DFL);
  signal(SIGUSR1, SIG_DFL);
  signal(SIGALRM, SIG_DFL);

  umask(0);
  if(setsid() < 0)
    exit(EXIT_FAILURE);
  
  stdin  = freopen("/dev/null", "r", stdin);
  stdout = freopen("/dev/null", "w", stdout);
  stderr = freopen("/dev/null", "w", stderr);
  
  kill(parent, SIGUSR1);
}

int main(int argc, char **argv)
{
  log_t* loghandle;
  void*  libhandle;
  int    retcode;

  char server_addr[256];
  char listen_port[100];
  char server_port[100];
  char logfile[PATH_MAX];
  char pidfile[PATH_MAX];
  char libfile[PATH_MAX];
  int  debug_flag;

  handler_api_t handler_api;

  mcp_parse_arguments(argc, argv, listen_port, server_port,
		      server_addr, libfile, pidfile, logfile, &debug_flag);

  if(util_file_stat(pidfile) != 0) {
    fprintf(stderr, "%s: PID file %s exists! Not starting.\n", argv[0], pidfile);
    exit(EXIT_FAILURE);
  }

  libhandle = dlopen(libfile, RTLD_NOW);
  if(!libhandle) {
    fprintf(stderr, "%s: Could not load handler library: %s\n", argv[0], dlerror());
    exit(EXIT_FAILURE);
  }

  handler_api.handler_info     = dlsym(libhandle, "handler_info");
  handler_api.handler_startup  = dlsym(libhandle, "handler_startup");
  handler_api.handler_shutdown = dlsym(libhandle, "handler_shutdown");

  if(!handler_api.handler_info ||
     !handler_api.handler_startup || !handler_api.handler_shutdown) {
    fprintf(stderr, "%s: Specified handler library is invalid", argv[0]);
    dlclose(libhandle);
    exit(EXIT_FAILURE);
  }

  loghandle = log_open(logfile, debug_flag);
  if(!loghandle) {
    fprintf(stderr, "%s: Could not open log file: %s\n", argv[0], MCPROXY_LOGFILE);
    exit(EXIT_FAILURE);
  }
  log_set_default(loghandle);

  if(debug_flag == LOG_NOFLAGS)
    mcp_daemonize();

  signal(SIGTERM, sig_quit);
  signal(SIGINT, sig_quit);
  util_file_writepid(pidfile);

  retcode = core_main(server_addr, server_port, listen_port, debug_flag,
		      &handler_api);
  
  log_close(loghandle);
  dlclose(libhandle);
  unlink(pidfile);
  exit(retcode);
}
