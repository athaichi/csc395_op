#include <stdint.h>
#include <stdbool.h>

#include "stivale2.h"

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
  
  // Begin terminal output
  kprintf("Translating %p \n", address); 

  // Zero out bottom 12 bits of starting table
  uintptr_t start_page = page_table >> 12; 

  // Make the address manipulatable 
  uint32_t addr = (uint32_t)address; 

  // find hhdm tag (add to physical address) WHERE THE FUCK IS THE HDR?????
  // HOW DO I GET THE HHDM TAG??????
  // struct stivale2_struct_tag_hhdm* hhdm = find_tag(hdr, STIVALE2_STRUCT_TAG_HHDM_ID);
  
  // Find index of level 4 table
  uint32_t index4 = (addr << 18) >> 55; 

  pt_entry_t level4entry = start_page[index4]; 
  if (level4entry.present != 0) { // if the following table exists
    kprintf("    Level 4 (index %d of %x)\n", level4entry, &start_page);
    kprintf("    "); 

    if (level4entry.kernel == 1) { // if it is usable
      kprintf("usable "); 
    }

    if (level4entry.writable == 1) { // if it is writable
      kprintf("writable "); 
    }

    if (level4entry.no_execute == 0) { // if it is executable
      kprintf("executable "); 
    }

    // find next table
    uintptr_t level3page = level4entry.address; //+ hhhdm value?? 
    kprintf("-> %p\n", level3page); 
  }

  // Set up printed formatting
  kprintf("Translating %p \n", address); 
  //kprintf("    Level 4 (index %d of %x)\n", index, &page);
  //kprintf("    Level 3 (index %d of %x)\n", index, &page);
  //kprintf("    Level 2 (index %d of %x)\n", index, &page);
  //kprintf("    Level 1 (index %d of %x)\n", index, &page); 
}
