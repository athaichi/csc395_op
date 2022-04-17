#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

#include "memory.h"
#include "kprint.h"
// note to self: bottom bits are on the right


// create a page entry
// should be 64 bits
typedef struct page_table_entry {
  bool present : 1;
  bool writable : 1;
  bool user : 1;
  bool write_through : 1;
  bool cache_disable : 1;
  bool accessed : 1;
  bool dirty : 1;
  bool page_size : 1;
  uint8_t _unused0 : 4;
  uintptr_t address : 40;
  uint16_t _unused1 : 11;
  bool no_execute : 1;
} __attribute__((packed)) pt_entry_t;

// get cr3 register
uintptr_t read_cr3() {
  uintptr_t value;
  __asm__("mov %%cr3, %0" : "=r" (value));
  return value;
}

void write_cr3(uint64_t value) {
  __asm__("mov %0, %%cr3" : : "r" (value));
}

// create a helper to get physical to virtual 
uintptr_t get_hhdm(struct stivale2_struct* hdr) {
  struct stivale2_struct_tag_hhdm* hhdm = find_tag(hdr, STIVALE2_STRUCT_TAG_HHDM_ID);
  return (uintptr_t)hhdm->addr; 
}

/**
 * Translate a virtual address to its mapped physical address
 *
 * \param address     The virtual address to translate
 */
void translate(void* address, struct stivale2_struct* hdr) {

  // find starting table address
  uintptr_t table_phys = read_cr3() & 0xFFFFFFFFFFFFF000;

  // get hhdm tag 
  uintptr_t hhdm_base = (uintptr_t)(get_hhdm(hdr)); 

  // find the first table 
  pt_entry_t * table = (pt_entry_t *)(table_phys + hhdm_base); 

  // make parameter actually usable
  uint64_t linear_addr = (uint64_t)address; 

  // separate linear address into index pieces
  uint16_t indices[] = {
    linear_addr & 0xFFF,         // offset
    (linear_addr >> 12) & 0x1FF, // level 1
    (linear_addr >> 21) & 0x1FF, // level 2
    (linear_addr >> 30) & 0x1FF, // level 3
    (linear_addr >> 39) & 0x1FF, // level 4
  };


  kprintf("og address: %p", address);
  for(int i = 4; i >= 0 ; i--) {
    kprintf("\n   level %d index is: %p", i, indices[i]); 
  }
  kprint_c('\n'); 
  
  // Begin terminal output
  kprintf("Translating %p \n", address); 

  for (int i = 4; i > 0; i--) {
    uint16_t index = indices[i]; 
    if (table[index].present) {
      kprintf("   level %d (index %d of %p)\n", i, index, table); 
      kprintf("      %s", table[index].user ? "user " : "kernel "); 
      kprintf("%s", table[index].writable ? "writable " : ""); 
      kprintf("%s", table[index].no_execute ? "" : "executable"); 

      // Get physical address of the next level table
      table_phys = table[index].address << 12; 
      table = (pt_entry_t*)(table_phys + hhdm_base); 

      kprintf(" ->  %p\n", table_phys); 

    } else {
      kprintf("   Not Present!\n"); 
      return; 
    }
  }

  // get final offset 
  //if ()

  // get final offset 
  uintptr_t result = table_phys + indices[0]; 

  // print final offset address 
  kprintf("%p maps to %p\n", address, result); 

  return; 
}



// ===============================================
// ===============================================

// NOTE TO SELF: virtual address = physical address + hhdm address
//               physical address = virtual address - hhdm address

// global! gets set in init_init (in boot)
extern uintptr_t hhdm_base; 

void* ptov (uint64_t paddr) {
  uintptr_t vaddr = (uintptr_t)(paddr + hhdm_base); 
  return (void*)vaddr; 
}

// global! keep track of number of free pages - just for testing purposes
int free_page_counter = 0;

// do this via a linked list
typedef struct pmem_freeentry {
  struct pmem_freeentry * next;
} __attribute__((packed)) pmem_freeentry_t;  

// set up pointer to the front of the freelist
// this pointer is virtual memory
pmem_freeentry_t * freelist = NULL; 

/**
 * Allocate a page of physical memory.
 * \returns the physical address of the allocated physical memory or 0 on error.
 */
uintptr_t pmem_alloc() {

  // if the freelist is empty, then darn
  if (freelist == NULL) {
    kprintf("No free space, sorry :(\n"); 
    // halt(); 
    return 0; 
  }

  // give the first page on the freelist
  // change address to be actually physical instead of virtual
  uintptr_t allocated = (uintptr_t)(freelist) - hhdm_base; 

  // move freelist pointer to the next page in freelist
  freelist = freelist->next; 

  // update counter - just for testing!
  free_page_counter--; 

  return allocated; 
}

/**
 * Free a page of physical memory.
 * \param p is the physical address of the page to free, which must be page-aligned.
 */
void pmem_free(uintptr_t p) { 

  //kprintf("freeing %p, freelist: %p\n", p, freelist); 

  // create a new entry in the freelist and init it to be empty
  // this is virtual
  pmem_freeentry_t * new = NULL; 
  
  // add the entry to the beginning of the freelist
  // convert physical address to be stored virtually 
  new = (pmem_freeentry_t*)(p + hhdm_base); 
  new->next = freelist; 

  // move freelist pointer to new beginning node
  freelist = new; 

  // update counter - just for testing!
  free_page_counter++; 

}

// Unmap everything in the lower half of an address space with level 4 page table at address root
void unmap_lower_half(uintptr_t root) {
  // We can reclaim memory used to hold page tables, but NOT the mapped pages
  pt_entry_t* l4_table = ptov(root);
  for (size_t l4_index = 0; l4_index < 256; l4_index++) {

    // Does this entry point to a level 3 table?
    if (l4_table[l4_index].present) {

      // Yes. Mark the entry as not present in the level 4 table
      l4_table[l4_index].present = false;

      // Now loop over the level 3 table
      pt_entry_t* l3_table = ptov(l4_table[l4_index].address << 12);
      for (size_t l3_index = 0; l3_index < 512; l3_index++) {

        // Does this entry point to a level 2 table?
        if (l3_table[l3_index].present && !l3_table[l3_index].page_size) {

          // Yes. Loop over the level 2 table
          pt_entry_t* l2_table = ptov(l3_table[l3_index].address << 12);
          for (size_t l2_index = 0; l2_index < 512; l2_index++) {

            // Does this entry point to a level 1 table?
            if (l2_table[l2_index].present && !l2_table[l2_index].page_size) {

              // Yes. Free the physical page the holds the level 1 table
              pmem_free(l2_table[l2_index].address << 12);
            }
          }

          // Free the physical page that held the level 2 table
          pmem_free(l3_table[l3_index].address << 12);
        }
      }

      // Free the physical page that held the level 3 table
      pmem_free(l4_table[l4_index].address << 12);
    }
  }

  // Reload CR3 to flush any cached address translations
  write_cr3(read_cr3());
}


// this does a whole bunch of useful init stuff: 
/*
- sets up global hhdm base
- puts stuff in the freelist
*/
void mem_init(struct stivale2_struct* hdr) {
  // set global
  hhdm_base = (uintptr_t)(get_hhdm(hdr));
  //kprintf("hhdm is %p\n", hhdm_base);

  // put stuff in freelist
  struct stivale2_struct_tag_memmap* memmap = find_tag(hdr, STIVALE2_STRUCT_TAG_MEMMAP_ID);
  for(uint64_t current = 0; current < memmap->entries; current++) {
    struct stivale2_mmap_entry cur = memmap->memmap[current]; 
    
    // if section of memory is usable or bootloader reclaimable
    // usable/bootloader reclaimable memory is automatically 0x1000 size and page aligned per stivale2 specs
    //if((cur.type == 1) || (cur.type == 0x1000)) { 
    if (cur.type == 1) {
      // add it to the freelist
      //kprintf("found an entry\n");
      uint64_t curr, end = 0; 
      curr = cur.base;
      end = cur.base + cur.length; 
      //kprintf("start: %p, end: %p\n", curr, end); 
       while (curr < end) {
        pmem_free(curr); 
         //kprintf("subdivide...");
         curr += 0x1000; 
         free_page_counter++; 
         //while(1) {}; 
       } 
    }
  }

  // unmap the lower half of the address space, using cr3 register as root address
  unmap_lower_half(read_cr3()); 

  kprintf("number of free pages are: %d\n", free_page_counter); 
}

// makes the TLB ignore old addresses you used to have
void invalidate_tlb(uintptr_t virtual_address) {
   __asm__("invlpg (%0)" :: "r" (virtual_address) : "memory");
}

/**
 * Map a single page of memory into a virtual address space.
 * \param root The physical address of the top-level page table structure
 * \param address The virtual address to map into the address space, must be page-aligned
 * \param usable Should the page be user-accessible?
 * \param writable Should the page be writable?
 * \param executable Should the page be executable?
 * \returns true if the mapping succeeded, or false if there was an error
 */
bool vm_map(uintptr_t root, uintptr_t address, bool usable, bool writable, bool executable) {
  
  // separate virtual address into pieces
  uint16_t indices[] = {
    address & 0xFFF,         // offset
    (address >> 12) & 0x1FF, // level 1
    (address >> 21) & 0x1FF, // level 2
    (address >> 30) & 0x1FF, // level 3
    (address >> 39) & 0x1FF, // level 4
  };

  // create our physical table pointer
  uintptr_t table_phys = root; 

  // find the first table 
  pt_entry_t * table = (pt_entry_t *)(root + hhdm_base); 

  // go through the four level page table until we reach level 1
  for (int i = 4; i > 0; i--) {
    uint16_t index = indices[i]; 

    // check if table is there, if it is life is easy
    if (table[index].present) {

      // if we are at the last level of table, check if something is already there
      if (i == 1) {
        // yes there is - do not rewrite
        return false; 
      }

      // otherwise, get physical address of the next level table
      table_phys = table[index].address << 12; 
      table = (pt_entry_t*)(table_phys + hhdm_base); 

    } else { // table is not there, life is hard :(
      // try and add a new table (this is physical address) 
      uintptr_t new_table_phys = pmem_alloc(); 
      if (new_table_phys == 0) {
        // out of space, fail and return
        return false; 
      }

      // make it virtual to create a page table and cast
      pt_entry_t * new_table = (pt_entry_t*)(new_table_phys + hhdm_base); 

      // zero it out
      kmemset(new_table, 0, 0x1000);

      // update previous table to have the newly allocated table
      table[index].present = 1; 
      table[index].address = new_table_phys >> 12; 

      // if we're at the "last" level 
      if (i == 1) {
        // then set table to be params
        table[index].no_execute = !executable;
        table[index].user = usable;
        table[index].writable = writable; 

        // update tlb
        invalidate_tlb(address);

        // whoohooo! we did it, exit pls
        return true; 
      } 

      // we're not at the end
      // set new table to be readable, writable and user accessible and give address
      table[index].no_execute = 0;
      table[index].user = 1;
      table[index].writable = 1; 

      // try and get to the next table
      table_phys = table[index].address << 12; 
      table = (pt_entry_t*)(table_phys + hhdm_base); 
    }
  }

  // if you're here, you've failed somehow
  kprintf("(vm_map) idk how you've got here but you're not supposed to be here\n"); 
  return false; 
}

/**
 * Unmap a page from a virtual address space
 * \param root The physical address of the top-level page table structure
 * \param address The virtual address to unmap from the address space
 * \returns true if successful, or false if anything goes wrong
 */
bool vm_unmap(uintptr_t root, uintptr_t address) {
  // iterate through page table to address
  // separate virtual address into pieces
  uint16_t indices[] = {
    address & 0xFFF,         // offset
    (address >> 12) & 0x1FF, // level 1
    (address >> 21) & 0x1FF, // level 2
    (address >> 30) & 0x1FF, // level 3
    (address >> 39) & 0x1FF, // level 4
  };

  // create our physical table pointer
  uintptr_t table_phys = root; 

  // find the first table 
  pt_entry_t * table = (pt_entry_t *)(root + hhdm_base); 

  // go through the four level page table until we reach level 1
  for (int i = 4; i > 0; i--) {
    uint16_t index = indices[i]; 

    // check if table is there, if it is life is easy
    if (table[index].present) {

      // if we are at the last level of table, check if something is already there
      if (i == 1) {
        // there is! let's unmap
        pmem_free(table[index].address); 

        // page is no longer accessible in table
        table[index].present = false; 

        // update tlb
        invalidate_tlb(address);

        // we're done!
        return true; 
      }

      // otherwise, get physical address of the next level table
      table_phys = table[index].address << 12; 
      table = (pt_entry_t*)(table_phys + hhdm_base); 

    } else { // table is not there -> address is not mapped in the first place
      // oh no! something has gone wrong :(
      return false; 
    }
  }

  // if you're here, you've failed somehow
  kprintf("(vm_unmap) idk how you've got here but you're not supposed to be here\n"); 
  return false; 
}

/**
 * Change the protections for a page in a virtual address space
 * \param root The physical address of the top-level page table structure
 * \param address The virtual address to update
 * \param user Should the page be user-accessible or kernel only?
 * \param writable Should the page be writable?
 * \param executable Should the page be executable?
 * \returns true if successful, or false if anything goes wrong (e.g. page is not mapped)
 */
bool vm_protect(uintptr_t root, uintptr_t address, bool user, bool writable, bool executable){
  // separate virtual address into pieces
  uint16_t indices[] = {
    address & 0xFFF,         // offset
    (address >> 12) & 0x1FF, // level 1
    (address >> 21) & 0x1FF, // level 2
    (address >> 30) & 0x1FF, // level 3
    (address >> 39) & 0x1FF, // level 4
  };

  // create our physical table pointer
  uintptr_t table_phys = root; 

  // find the first table 
  pt_entry_t * table = (pt_entry_t *)(root + hhdm_base); 

  // go through the four level page table until we reach level 1
  for (int i = 4; i > 0; i--) {
    uint16_t index = indices[i]; 

    // check if table is there, if it is life is easy
    if (table[index].present) {

      // if we are at the last level of table, check if something is already there
      if (i == 1) {
        // yes there is time to change permissions
        table[index].no_execute = !executable; 
        table[index].user = user; 
        table[index].writable = writable; 

        // update tlb
        invalidate_tlb(address);

        // we've finished successfully!
        return true; 
      }

      // otherwise, get physical address of the next level table
      table_phys = table[index].address << 12; 
      table = (pt_entry_t*)(table_phys + hhdm_base); 

    } else { // table is not there -> address is not mapped in the first place
      // oh no! something has gone wrong :(
      return false; 
    }
      
  }

  // if you're here, you've failed somehow
  kprintf("(vm_protect) idk how you've got here but you're not supposed to be here\n"); 
  return false; 
}

