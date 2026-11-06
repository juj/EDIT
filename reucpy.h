#pragma once

#define RAM2REU 0x90 // Copy from C64 RAM to REU memory
#define REU2RAM 0x91 // Copy from REU memory to C64 RAM
#define REUSWAP 0x92 // Swap contents between REU and C64 RAM addresses
#define REUCMP  0x93 // Compare contents between REU and C64 RAM

typedef unsigned _BitInt(24) reu_addr_t;

// Like memcpy, but copies data between C64 RAM and REU.
// size: number of bytes to write. Note that 0 is interpreted as 65536 bytes!
// direction: specifies the direction of copy. Use one of the defines above.
void reucpy(void *c64_addr, reu_addr_t reu_addr, uint16_t size, uint8_t direction);

// Like memset, for setting data in REU memory.
// reu_addr: Starting byte address in REU memory.
// val: the byte value to fill.
// size: the number of bytes to fill. Note that 0 is interpreted as 65536 bytes!
void reuset(reu_addr_t reu_addr, uint8_t val, uint16_t size);

// Initializes REU, and returns the number of 64KB REU banks that were detected:
// 0 - no REU detected.
// 1 - 64KB REU detected,
// 256: 16MB REU detected.
// Call this once before calling reucpy().
int reu_init(void);
