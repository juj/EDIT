#pragma once

#define RAM2REU 0x90
#define REU2RAM 0x91
#define REUSWAP 0x92
#define REUCMP  0x93

typedef unsigned _BitInt(24) reu_addr_t;
void reucpy(void *c64_addr, reu_addr_t reu_addr, uint16_t size, uint8_t direction);

// Returns the number of 64KB REU banks that were detected:
// 0 - no REU detected.
// 1 - 64KB REU detected,
// 256: 16MB REU detected.
int reu_detect(void);
