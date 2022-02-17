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

uintptr_t print_abilites(pt_entry_t page, uint32_t index, int level) {
  
  kprintf("    Level %d (index %d of %p)\n", level, index, page);
  kprintf("    "); 

  if (page.kernel == 1) { // if it is usable
    kprintf("usable "); 
  }

  if (page.writable == 1) { // if it is writable
    kprintf("writable "); 
  }

  if (page.no_execute == 0) { // if it is executable
    kprintf("executable "); 
  }

  // find next table
  uintptr_t next_page = page.address; //+ hhhdm value?? 
  kprintf("-> %p\n", &next_page); 

  return next_page; 
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
  uint32_t index4 = (addr << 17) >> 56; 

  pt_entry_t level4entry = start_page[index4]; 
  if (level4entry.present != 0) {
    uintptr_t level3page = print_abilites(level4entry, index4, 4); 
    
    level3page >> 12; 
    uint32_t index3 = (addr << 26) >> 56; 
    pt_entry_t level3entry = level3page[index3]; 
    if(level3entry.present != 0) {
      uintptr_t level2page = print_abilites(level3entry, index3, 3); 
    
      level2page >> 12; 
      uint32_t index2 = (addr << 35) >> 56; 
      pt_entry_t level2entry = level2page[index2];
      if(level2entry.present != 0) {
        uintptr_t level1page = print_abilites(level2entry, index2, 2); 
    
        level1page >> 12; 
        uint32_t index1 = (addr << 47) >> 56; 
        pt_entry_t offset_entry = level1page[index1];
        if (offset_entry.present != 0) {
          uint32_t final_dest = (addr << 53) >> 53;
          uintptr_t physical_address = offset_entry[final_dest]; 
          kprintf("%p maps to %p\n", address, physical_address); 
        } else {
          kprintf("    Level 1 (index %d of %p) is not used\n", final)
        }
      }
    }
  }
  

  // Set up printed formatting 
  //kprintf("    Level 4 (index %d of %x)\n", index, &page);
  //kprintf("    Level 3 (index %d of %x)\n", index, &page);
  //kprintf("    Level 2 (index %d of %x)\n", index, &page);
  //kprintf("    Level 1 (index %d of %x)\n", index, &page); 
}
