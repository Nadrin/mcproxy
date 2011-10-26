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
#include <signal.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <config.h>
#include <system.h>
#include <log.h>
#include <core.h>
#include <util.h>

// Global quit flag and signal handler
extern volatile sig_atomic_t mcp_quit;
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
  fprintf(stderr, "usage: %s [-d] [-p port] [-l logfile] [-r pidfile] <-L handlerlib> [server_addr] [server_port]\n", progname);
  exit(EXIT_FAILURE);
}

static void mcp_parse_arguments(int argc, char **argv, sys_config_t* config)
{
  int c, i;
  char  workdir[PATH_MAX];
  char* homedir = getenv("HOME");

  strcpy(config->listen_port, MCPROXY_DEFAULT_LPORT);
  strcpy(config->server_port, MCPROXY_DEFAULT_PORT);
  strcpy(config->server_addr, MCPROXY_DEFAULT_HOST);

  config->debug = LOG_NOFLAGS;

  getcwd(workdir, PATH_MAX);
  sprintf(config->pidfile, "%s/%s", homedir?homedir:"/tmp", MCPROXY_PIDFILE);
  sprintf(config->logfile, "%s/%s", homedir?homedir:"/tmp", MCPROXY_LOGFILE);
  config->libfile[0] = 0;

  while((c = getopt(argc, argv, "+dp:l:r:L:")) != -1) {
    switch(c) {
    case 'd':
      config->debug = LOG_DEBUG;
      break;
    case 'p':
      strcpy(config->listen_port, optarg);
      break;
    case 'l':
      if(optarg[0] == '/')
        strcpy(config->logfile, optarg);
      else
        sprintf(config->logfile, "%s/%s", workdir, optarg);
      break;
    case 'r':
      if(optarg[0] == '/')
        strcpy(config->pidfile, optarg);
      else
        sprintf(config->pidfile, "%s/%s", workdir, optarg);
      break;
    case 'L':
      if(optarg[0] == '/')
        strcpy(config->libfile, optarg);
      else
        sprintf(config->libfile, "%s/%s", workdir, optarg);
      break;
    default:
      mcp_usage(argv[0]);
    }
  }
  if(!*(config->libfile))
    mcp_usage(argv[0]);
  if(optind > 0 && strcmp(argv[optind-1], "--") == 0)
    optind--;

  i=0;
  while(optind < argc && strcmp(argv[optind], "--") != 0) {
    if(i==0) strcpy(config->server_addr, argv[optind]);
    if(i==1) strcpy(config->server_port, argv[optind]);
    if(i==2) mcp_usage(argv[0]);
    i++; optind++;
  }

  if(++optind < argc)
    sys_set_args(argc - optind, &argv[optind]);
  if(!homedir)
    fprintf(stderr, "%s: Warning, no HOME environment variable defined!\n", argv[0]);
}

static void mcp_daemonize(void)
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

  handler_api_t handler_api;
  sys_config_t* system_config;
  
  system_config = sys_get_config();
  mcp_parse_arguments(argc, argv, system_config);
  
  if(system_config->debug == LOG_NOFLAGS && 
     util_file_stat(system_config->pidfile) != 0) {
    fprintf(stderr, "%s: PID file %s exists! Not starting.\n", argv[0], system_config->pidfile);
    exit(EXIT_FAILURE);
  }
  
  libhandle = dlopen(system_config->libfile, RTLD_NOW);
  if(!libhandle) {
    fprintf(stderr, "%s: Could not load handler library: %s\n", argv[0], dlerror());
    exit(EXIT_FAILURE);
  }

  if(sys_initapi(libhandle, &handler_api) != 0) {
    fprintf(stderr, "%s: Specified handler library is invalid", argv[0]);
    dlclose(libhandle);
    exit(EXIT_FAILURE);
  }
  
  loghandle = log_open(system_config->logfile, system_config->debug);
  if(!loghandle) {
    fprintf(stderr, "%s: Could not open log file: %s\n", argv[0], MCPROXY_LOGFILE);
    dlclose(libhandle);
    exit(EXIT_FAILURE);
  }
  log_set_default(loghandle);

  if(system_config->debug == LOG_NOFLAGS)
    mcp_daemonize();

  signal(SIGTERM, sig_quit);
  signal(SIGINT, sig_quit);

  if(system_config->debug == LOG_NOFLAGS)
    util_file_writepid(system_config->pidfile);

  retcode = core_main(system_config, &handler_api);
  
  log_close(loghandle);
  dlclose(libhandle);
  if(system_config->debug == LOG_NOFLAGS)
    unlink(system_config->pidfile);

  exit(retcode);
}
