/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak, Dylan Lukes
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#ifndef __MCPROXY_ENDIAN_H
#define __MCPROXY_ENDIAN_H

typedef enum { MCP_UNKNOWN_ENDIAN = 0x00, MCP_LITTLE_ENDIAN = 0x01, MCP_BIG_ENDIAN = 0x02} mcp_byte_order_t;

static volatile mcp_byte_order_t mcp_byte_order;

void mcp_test_byte_order(void);

int16_t mcp_swaps(int16_t);
int32_t mcp_swapl(int32_t);
int64_t mcp_swapq(int64_t);

int16_t mcp_htons(int16_t);
int32_t mcp_htonl(int32_t);
int64_t mcp_htonq(int64_t);

int16_t mcp_ntohs(int16_t);
int32_t mcp_ntohl(int32_t);
int64_t mcp_ntohq(int64_t);

#endif
