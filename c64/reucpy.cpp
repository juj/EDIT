#include "reucpy.h"

static volatile uint8_t *const reu_status = (uint8_t*)0xDF00;
static volatile uint8_t *const reu_command = (uint8_t*)0xDF01;
static volatile uintptr_t *const reu_c64_addr = (uintptr_t*)0xDF02;
static volatile reu_addr_t *const reu_cart_addr = (reu_addr_t*)0xDF04;
static volatile uint16_t *const reu_xfer_length = (uint16_t*)0xDF07;
static volatile uint8_t *const reu_address_ctl = (uint8_t*)0xDF0A;

void reucpy(void *c64_addr, unsigned _BitInt(24) reu_addr, uint16_t size, uint8_t direction)
{
  assert(direction == RAM2REU || direction == REU2RAM);
  // Call reu_detect() first if this assert() triggers, or if manually
  // having done REU accesses in between, be sure to reset bits 7 and 6
  // of REU address control register back to zero.
  assert((*reu_address_ctl & 0xC0) == 0);

  *reu_c64_addr = (uintptr_t)c64_addr;
  *reu_cart_addr = reu_addr;
  *reu_xfer_length = size;
  *reu_command = direction;
}

int reu_detect()
{
  *reu_address_ctl = *reu_address_ctl & ~0xC0; // Make both C64 and REU addresses advance per each transferred byte

  char data[5] = { 'a', 'r', 'e', 'u', 0 };
  for(int bank = 255; bank >= 0; --bank)
  {
    data[0] = (uint8_t)bank;
    reucpy(data, (reu_addr_t)bank << 16, 4, RAM2REU);
  }
  memset(data, 0, sizeof(data));
  for(int bank = 0; bank < 256; ++bank)
  {
    reucpy(data, (reu_addr_t)bank << 16, 4, REU2RAM);
    if (data[0] != bank || data[1] != 'r' || data[2] != 'e' || data[3] != 'u') return bank;
  }
  return 256; // Full 16MB of REU available
}
