/* Minecraft Protocol Proxy (mcproxy)
 * Copyright (c) 2011 Michał Siejak
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

#define PROTOCOL_VERSION 15
#define _defmsg(id, format) { id, format, NULL, NULL, NULL, NULL }

msgdesc_t msgtable[] = {
  _defmsg(0x00, "i"), // Keep Alive
  _defmsg(0x01, "itliccc"), // Login Request
  _defmsg(0x02, "t"), // Handshake
  _defmsg(0x03, "t"), // Chat message
  _defmsg(0x04, "l"), // Time update
  _defmsg(0x05, "isss"), // Entity equipment
  _defmsg(0x06, "iii"), // Spawn position
  _defmsg(0x07, "iic"), // Use entity
  _defmsg(0x08, "ssf"), // Update health
  _defmsg(0x09, "ccsl"), // Respawn
  _defmsg(0x0A, "c"), // Player
  _defmsg(0x0B, "ddddc"), // Player position
  _defmsg(0x0C, "ffc"), // Player look
  _defmsg(0x0D, "ddddffc"), // Player position and look
  _defmsg(0x0E, "cicic"), // Player digging
  _defmsg(0x0F, "icics--"), // Player block placement (datahelper)
  _defmsg(0x10, "s"), // Holding change
  _defmsg(0x11, "icici"), // Unknown (NEW IN 1.3)
  _defmsg(0x12, "ic"), // Animation
  _defmsg(0x13, "ic"), // Entity action
  _defmsg(0x14, "itiiiccs"), // Named entity spawn
  _defmsg(0x15, "iscsiiiccc"), // Pickup spawn
  _defmsg(0x16, "ii"), // Collect item
  _defmsg(0x17, "iciiii---"), // Add object/vehicle (datahelper)
  _defmsg(0x18, "iciiiccm"), // Mob spawn
  _defmsg(0x19, "itiiii"), // Painting
  _defmsg(0x1B, "ffffcc"), // Unknown (NEW IN 1.3)
  _defmsg(0x1C, "isss"), // Entity velocity
  _defmsg(0x1D, "i"), // Destroy entity
  _defmsg(0x1E, "i"), // Entity
  _defmsg(0x1F, "iccc"), // Entity relative move
  _defmsg(0x20, "icc"), // Entity look
  _defmsg(0x21, "iccccc"), // Entity look and relative move
  _defmsg(0x22, "iiiicc"), // Entity teleport
  _defmsg(0x26, "ic"), // Entity status
  _defmsg(0x27, "ii"), // Attach entity
  _defmsg(0x28, "im"), // Entity metadata
  _defmsg(0x29, "iccs"), // Entity effect
  _defmsg(0x2A, "ic"), // Remove entity effect
  _defmsg(0x2B, "ccs"), // Experience
  _defmsg(0x32, "iic"), // Pre-chunk
  _defmsg(0x33, "isiccci"), // Map-chunk (datahelper)
  _defmsg(0x34, "iis"), // Multiblock change (datahelper)
  _defmsg(0x35, "icicc"), // Block change
  _defmsg(0x36, "isicc"), // Play note block
  _defmsg(0x3C, "dddfi"), // Explosion (datahelper)
  _defmsg(0x3D, "iicii"), // Door change (new in 1.6)
  _defmsg(0x46, "cc"), // New/Invalid state
  _defmsg(0x47, "iciii"), // Weather
  _defmsg(0x64, "ccuc"), // Open window
  _defmsg(0x65, "c"), // Close window
  _defmsg(0x66, "cscscs--"), // Window click (datahelper)
  _defmsg(0x67, "css--"), // Set slot (datahelper)
  _defmsg(0x68, "cs"), // Window items (datahelper)
  _defmsg(0x69, "css"), // Update progress bar
  _defmsg(0x6A, "csc"), // Transaction
  _defmsg(0x6B, "ssss"), // Creative inventory action
  _defmsg(0x82, "isitttt"), // Update sign
  _defmsg(0x83, "ssc"), // Map data (new in 1.6) (datahelper)
  _defmsg(0xC8, "ic"), // Increment statistic
  _defmsg(0xC9, "tcs"), // User list item
  _defmsg(0xFE, NULL), // Server list ping
  _defmsg(0xFF, "t"), // Kick
};
