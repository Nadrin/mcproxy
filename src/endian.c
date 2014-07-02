/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak, Dylan Lukes
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdint.h>

#include <endian.h>

inline void mcp_test_byte_order(void)
{
	if(!(mcp_byte_order == MCP_UNKNOWN_ENDIAN)) return;

	union {
		uint64_t i64;
		uint8_t i8[8];
	} test = {0x0706050403020100ull};

	if(test.i8[0] == 0){
		mcp_byte_order = MCP_LITTLE_ENDIAN;
	} else {
		mcp_byte_order = MCP_BIG_ENDIAN;
	}
}

inline int16_t mcp_swaps(int16_t x)
{
	return ((x & 0xff00) >> 8) |
           ((x & 0x00ff) << 8) ;
}

inline int32_t mcp_swapl(int32_t x)
{
	return ((x & 0xff000000) >> 24) |
           ((x & 0x00ff0000) >>  8) |
           ((x & 0x0000ff00) <<  8) |
           ((x & 0x000000ff) << 24) ;
}

inline int64_t mcp_swapq(int64_t x)
{
    return ((x & 0xff00000000000000ULL) >> 56) |
           ((x & 0x00ff000000000000ULL) >> 40) |
           ((x & 0x0000ff0000000000ULL) >> 24) |
           ((x & 0x000000ff00000000ULL) >>  8) |
           ((x & 0x00000000ff000000ULL) <<  8) |
           ((x & 0x0000000000ff0000ULL) << 24) |
           ((x & 0x000000000000ff00ULL) << 40) |
           ((x & 0x00000000000000ffULL) << 56) ;
}

inline int16_t mcp_htons(int16_t host_short)
{
  mcp_test_byte_order();
	if(mcp_byte_order == MCP_LITTLE_ENDIAN) host_short = mcp_swaps(host_short);
	return host_short;
}

inline int32_t mcp_htonl(int32_t host_long)
{
  mcp_test_byte_order();
	if(mcp_byte_order == MCP_LITTLE_ENDIAN) host_long = mcp_swapl(host_long);
	return host_long;
}

inline int64_t mcp_htonq(int64_t host_quad)
{
  mcp_test_byte_order();
	if(mcp_byte_order == MCP_LITTLE_ENDIAN) host_quad = mcp_swapq(host_quad);
	return host_quad;
}

inline int16_t mcp_ntohs(int16_t net_short)
{
  mcp_test_byte_order();
	if(mcp_byte_order == MCP_LITTLE_ENDIAN) net_short = mcp_swaps(net_short);
	return net_short;
}
inline int32_t mcp_ntohl(int32_t net_long)
{
  mcp_test_byte_order();
	if(mcp_byte_order == MCP_LITTLE_ENDIAN) net_long = mcp_swapl(net_long);
	return net_long;
}

inline int64_t mcp_ntohq(int64_t net_quad)
{
  mcp_test_byte_order();
	if(mcp_byte_order == MCP_LITTLE_ENDIAN) net_quad = mcp_swapq(net_quad);
	return net_quad;
}