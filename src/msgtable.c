/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak, Dylan Lukes
 *
 * Licensed under MIT open-source license.
 * See COPYING file for details.
 */

#include <stdlib.h>

#include <config.h>
#include <log.h>
#include <network.h>
#include <proto.h>
#include <proxy.h>

#define PROTOCOL_VERSION 29

#define _defmsg(id, format) { id, format, NULL, NULL, NULL, NULL }

// Protocol messages
const msgdesc_t msgtable[] = {
  _defmsg(0x00, "i"), // Keep Alive
  _defmsg(0x01, "ittiiccc"), // Login Request
  _defmsg(0x02, "t"), // Handshake
  _defmsg(0x03, "t"), // Chat message
  _defmsg(0x04, "l"), // Time update
  _defmsg(0x05, "isss"), // Entity equipment
  _defmsg(0x06, "iii"), // Spawn position
  _defmsg(0x07, "iic"), // Use entity
  _defmsg(0x08, "ssf"), // Update health
  _defmsg(0x09, "iccst"), // Respawn
  _defmsg(0x0A, "c"), // Player
  _defmsg(0x0B, "ddddc"), // Player position
  _defmsg(0x0C, "ffc"), // Player look
  _defmsg(0x0D, "ddddffc"), // Player position and look
  _defmsg(0x0E, "cicic"), // Player digging
  _defmsg(0x0F, "icic-"), // Player block placement (slot datahelper)
  _defmsg(0x10, "s"), // Holding change
  _defmsg(0x11, "icici"), // Use bed
  _defmsg(0x12, "ic"), // Animation
  _defmsg(0x13, "ic"), // Entity action
  _defmsg(0x14, "itiiiccs"), // Named entity spawn
  _defmsg(0x15, "iscsiiiccc"), // Pickup spawn
  _defmsg(0x16, "ii"), // Collect item
  _defmsg(0x17, "iciiii---"), // Add object/vehicle (datahelper)
  _defmsg(0x18, "iciiicccm"), // Mob spawn
  _defmsg(0x19, "itiiii"), // Painting
  _defmsg(0x1A, "iiiis"), // Unknown
  _defmsg(0x1C, "isss"), // Entity velocity
  _defmsg(0x1D, "i"), // Destroy entity
  _defmsg(0x1E, "i"), // Entity
  _defmsg(0x1F, "iccc"), // Entity relative move
  _defmsg(0x20, "icc"), // Entity look
  _defmsg(0x21, "iccccc"), // Entity look and relative move
  _defmsg(0x22, "iiiicc"), // Entity teleport
  _defmsg(0x23, "ic"), // Entity head look
  _defmsg(0x26, "ic"), // Entity status
  _defmsg(0x27, "ii"), // Attach entity
  _defmsg(0x28, "im"), // Entity metadata
  _defmsg(0x29, "iccs"), // Entity effect
  _defmsg(0x2A, "ic"), // Remove entity effect
  _defmsg(0x2B, "fss"), // Experience
  _defmsg(0x32, "iic"), // Pre-chunk
  _defmsg(0x33, "iicssii"), // Map-chunk (datahelper)
  _defmsg(0x34, "iisi"), // Multiblock change (datahelper)
  _defmsg(0x35, "icicc"), // Block change
  _defmsg(0x36, "isicc"), // Play note block
  _defmsg(0x3C, "dddfi"), // Explosion (datahelper)
  _defmsg(0x3D, "iicii"), // Door change
  _defmsg(0x46, "cc"), // New/Invalid state
  _defmsg(0x47, "iciii"), // Weather
  _defmsg(0x64, "cctc"), // Open window
  _defmsg(0x65, "c"), // Close window
  _defmsg(0x66, "cscsc-"), // Window click (slot datahelper)
  _defmsg(0x67, "cs-"), // Set slot (slot datahelper)
  _defmsg(0x68, "cs-"), // Window items (slot array datahelper)
  _defmsg(0x69, "css"), // Update window property
  _defmsg(0x6A, "csc"), // Transaction
  _defmsg(0x6B, "s-"), // Creative inventory action (slot datahelper)
  _defmsg(0x6C, "cc"), // Enchant item
  _defmsg(0x82, "isitttt"), // Update sign
  _defmsg(0x83, "ssc"), // Map data (datahelper)
  _defmsg(0x84, "isiciii"), // Update tile entity
  _defmsg(0xC8, "ic"), // Increment statistic
  _defmsg(0xC9, "tcs"), // User list item
  _defmsg(0xCA, "cccc"), // Player abilities
  _defmsg(0xFA, "ts"), // Plugin data (datahelper)
  _defmsg(0xFE, NULL), // Server list ping
  _defmsg(0xFF, "t"), // Kick
};

// Enchantable item IDs
const short eidtable[] = {
  0x103, // Flint and steel
  0x105, // Bow
  0x15A, // Fishing rod
  0x167, // Shears
 
  // TOOLS
  // sword, shovel, pickaxe, axe, hoe
  0x10C, 0x10D, 0x10E, 0x10F, 0x122, // WOOD
  0x110, 0x111, 0x112, 0x113, 0x123, // STONE
  0x10B, 0x100, 0x101, 0x102, 0x124, // IRON
  0x114, 0x115, 0x116, 0x117, 0x125, // DIAMOND
  0x11B, 0x11C, 0x11D, 0x11E, 0x126, // GOLD
 
  // ARMOUR
  // helmet, chestplate, leggings, boots
  0x12A, 0x12B, 0x12C, 0x12D, // LEATHER
  0x12E, 0x12F, 0x130, 0x131, // CHAIN
  0x132, 0x133, 0x134, 0x135, // IRON
  0x136, 0x137, 0x138, 0x139, // DIAMOND
  0x13A, 0x13B, 0x13C, 0x13D, // GOLD

  0
};
