#ifndef __MCPROXY_API_H
#define __MCPROXY_API_H

// API entry function prototypes for handler libraries
handler_info_t* handler_info(void);
int             handler_startup(msgdesc_t* msglookup, event_t* events);
int             handler_shutdown(void);

#endif
