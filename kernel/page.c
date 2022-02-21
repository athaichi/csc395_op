#include <stdint.h>
#include <stdbool.h>

#include "kprint.h"

// create a page entry
// should be 64 bits
typedef struct page_table_entry {
  bool present : 1;
  bool writable : 1;
  bool kernel : 1;      // also known as usable
  uint16_t unused : 9;
  uint64_t address : 51;
  bool no_execute : 1;
} __attribute__((packed)) pt_entry_t;

// get cr3 register
uintptr_t read_cr3() {
  uintptr_t value;
  __asm__("mov %%cr3, %0" : "=r" (value));
  return value;
}

/**
 * Translate a virtual address to its mapped physical address
 *
 * \param address     The virtual address to translate
 */
void translate(uintptr_t page_table, void* address) {
  //uintptr_t start_page = read_cr3(page_table) >> 12; // zero out bottom 12 bits

  // Set up printed formatting
  kprintf("Translating %x \n", &address); 
  //kprintf("    Level 4 (index %d of %x)\n", index, &page);
  //kprintf("    Level 3 (index %d of %x)\n", index, &page);
  //kprintf("    Level 2 (index %d of %x)\n", index, &page);
  //kprintf("    Level 1 (index %d of %x)\n", index, &page); 
}
