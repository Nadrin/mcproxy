/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
 * ServerLog handler library.
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_HANDLERS_TRANSACTION_H
#define __MCPROXY_HANDLERS_TRANSACTION_H

int transact_handler_main(cid_t client_id, char direction, unsigned char msg_id,
			  nethost_t* hfrom, nethost_t* hto, objlist_t* data,
			  void* extra);

#endif
