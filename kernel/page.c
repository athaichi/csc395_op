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
void translate(void* address) {

  uintptr_t root = read_cr3() & 0xFFFFFFFFFFFFF000

  // Create an array of indices: 
  uint16_t indices[] = {

  }
  
  // Begin terminal output
  kprintf("Translating %p \n", address); 

  for (int i = 4; i > 0; i--) {
    uint16_t index = indices[i]; 
    if (table[index].present) {
      kprintf("   level %d : %s", i, table[index].user ? "user" : "kernel"); 
      kprintf(" %s", table[index].writable ? "writable" : ""); 
      kprintf(" %s", table[index].no_execute ? "" : "executable"); 

      // Get physical address of the next level table
      table_phys = table[index].address << 12; 
      table = (pt_entry_t*)(table_phys + hhdm_base); 

      kprintf(" ->  %p", table_phys); 

    } else {
      kprintf("   Not Present!\n"); 
      return; 
    }
  }

  // get final offset 
  uintptr_t result = table_phys + indices[0]; 

  // print final offset address 
  printf("%p maps to %p\n", addr, result); 

  return; 
}
