#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "stivale2.h"
#include "util.h"
#include "interrupts.h"
#include "page.h"
#include "kprint.h"
#include "memory.h"
#include "pic.h"
#include "port.h"
#include "systemcalls.h"



// Reserve space for the stack
static uint8_t stack[8192];


// for testing interrupts
static struct stivale2_tag unmap_null_hdr_tag = {
  .identifier = STIVALE2_HEADER_TAG_UNMAP_NULL_ID,
  .next = 0
};


// Request a terminal from the bootloader
static struct stivale2_header_tag_terminal terminal_hdr_tag = {
	.tag = {
    .identifier = STIVALE2_HEADER_TAG_TERMINAL_ID,
    .next = (uintptr_t)&unmap_null_hdr_tag
  },
  .flags = 0
};

// Declare the header for the bootloader
__attribute__((section(".stivale2hdr"), used))
static struct stivale2_header stivale_hdr = {
  // Use ELF file's default entry point
  .entry_point = 0,

  // Use stack (starting at the top)
  .stack = (uintptr_t)stack + sizeof(stack),

  // Bit 1: request pointers in the higher half
  // Bit 2: enable protected memory ranges (specified in PHDR)
  // Bit 3: virtual kernel mappings (no constraints on physical memory)
  // Bit 4: required
  .flags = 0x1E,
  
  // First tag struct
  .tags = (uintptr_t)&terminal_hdr_tag
};

typedef void (*term_write_t)(const char*, size_t);
term_write_t term_write = NULL;

void term_setup(struct stivale2_struct* hdr) {
  // Look for a terminal tag
  struct stivale2_struct_tag_terminal* tag = find_tag(hdr, STIVALE2_STRUCT_TAG_TERMINAL_ID);

  // Make sure we find a terminal tag
  if (tag == NULL) halt();

  // Save the term_write function pointer
	term_write = (term_write_t)tag->term_write;
}

// --------------------------------------------------
// =======================================================
// =======================================================

// interrupt for key presses
__attribute__((interrupt))
void keyboard_interrupt(interrupt_context_t* ctx) {
  // get the keycode
  uint8_t code = inb(0x60);

  // make it the actual character
  char key = getkey(code);  

  // FOR TESTING
  // print character
  //kprint_c(key); 

  // write character to buffer
  write(key); 

  outb(PIC1_COMMAND, PIC_EOI); 
// ==============================================
// ==============================================

#include <stdbool.h>
// note to self: bottom bits are on the right

// create a page entry
// should be 64 bits
typedef struct page_table_entry {
  bool present : 1;
  bool writable : 1;
  bool user : 1;      // also known as usable
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

// global! gets set in mem_init
uintptr_t hhdm_base = 0; 

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
      kprintf("found an entry\n");
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

  kprintf("number of free pages are: %d", free_page_counter); 
}

// credit: https://aticleworld.com/memset-in-c/ 
void k_memset(void *arr, uint32_t c, size_t len) {
    uint8_t *current = arr; 
    for (size_t  i = 0; i < len; i++) {
        current[i] = c; 
    }

    return; 
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
      k_memset(new_table, 0, 0x1000);

      // update previous table to have the newly allocated table
      table[index].present = 1; 
      table[index].address = new_table_phys >> 12; 

      // if we're at the "last" level 
      if (i == 1) {
        // then set table to be params
        table[index].no_execute = !executable;
        table[index].user = usable;
        table[index].writable = writable; 

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

uint64_t read_cr0() {
  uintptr_t value;
  __asm__("mov %%cr0, %0" : "=r" (value));
  return value;
}

void write_cr0(uint64_t value) {
  __asm__("mov %0, %%cr0" : : "r" (value));
}

 

// -------------------------------------------------

void _start(struct stivale2_struct* hdr) {
  // We've booted! Let's start processing tags passed to use from the bootloader
  term_setup(hdr);
  //idt_setup(); 

  // Keyboard stuff
  pic_init(); 
  pic_unmask_irq(1); 

  // set up customized handler functions: 
  // allow us to make system calls!
  idt_set_handler(0x80, syscall_entry, IDT_TYPE_TRAP);
  // allow us to handle keyboard interrupts
  idt_set_handler(IRQ1_INTERRUPT, keyboard_interrupt, IDT_TYPE_TRAP);

  // Print a greeting
  term_write("Hello Kernel!\n", 14);

  //all_tests(); 

  // test usable_memory
  //usable_memory(hdr); 

  // test idt
  //  int* p = (int*)0x1;
  //  *p = 123; 
  //__asm__("int $2");


  // test paging
  //translate(read_cr3(), _start); 

  // test kgetc()
  // while(1) {
  //   kprintf("%c", kgetc()); 
  // }

  // test read
  char buf[6] = "hello";
  long rc = syscall(SYS_READ, 0, buf, 5);
  if (rc < 0) {
    kprintf("read failed\n");
  } else {
    buf[rc] = '\0';
    kprintf("read '%s'\n", buf);
  }

  // test write
  char buf2[6]; 
  long rc2 = syscall(SYS_WRITE, 'h', buf2, 1); 
  if (rc2 < 0) {
    kprintf("write failed\n"); 
  } else {
    rc = syscall(SYS_READ, 0, buf2, 5); 
    buf2[rc] = '\0';
    kprintf("wrote '%s'\n", buf2); 
  }
  //translate(_start, hdr);
  //translate(stack, hdr);  
  //translate(usable_memory, hdr);
  //translate(NULL, hdr); 

  // kprintf("interrupt should be above this\n"); 

  mem_init(hdr);
  kprintf("init finished\n");

  // test vm_map()
  // Enable write protection
  uint64_t cr0 = read_cr0();
  cr0 |= 0x10000;
  write_cr0(cr0);
  //    with unmapped addresses
  uintptr_t root = read_cr3() & 0xFFFFFFFFFFFFF000;
  int* p = (int*)0x50004000;
  bool result = vm_map(root, (uintptr_t)p, false, true, false);
  if (result) {
    *p = 123;
    kprintf("Stored %d at %p, number of free pages is now %d\n", *p, p, free_page_counter);
  } else {
   kprintf("vm_map failed with an error, freepage number unchanged %d\n", free_page_counter);
  }

  p = (int*)0x54739500; 
  result = vm_map(root, (uintptr_t)p, false, true, false);
  if (result) {
    *p = 123;
    kprintf("Stored %d at %p, number of free pages is now %d\n", *p, p, free_page_counter);
  } else {
   kprintf("vm_map failed with an error, freepage number unchanged %d\n", free_page_counter);
  }

  //    with mapped address
  result = vm_map(root, (uintptr_t)p, false, true, false);
  if (result) {
    *p = 123;
    kprintf("Stored %d at %p, number of free pages is now %d\n", *p, p, free_page_counter);
  } else {
   kprintf("vm_map failed with an error, freepage number unchanged %d\n", free_page_counter);
  }

  // test unmap
  //    with mapped entry
  result = vm_unmap(root, (uintptr_t)p); // unmapping address 0x54739500
  if (result) {
    kprintf("Removed %d at %p, number of free pages is now %d\n", *p, p, free_page_counter);
  } else {
   kprintf("vm_unmap failed with an error\n");
  }

  //    with unmapped entry
  p = (int*)0x500430020; 
  result = vm_unmap(root, (uintptr_t)p); 
  if (result) {
    kprintf("Removed %d at %p, number of free pages is now %d\n", *p, p, free_page_counter);
  } else {
   kprintf("vm_unmap failed with an error, free page num unchanged %d\n", free_page_counter);
  }

  // test protect - but badly
  //    with mapped value
  p = (int*)0x50004000; // from first vm_map test
  result = vm_protect(root, (uintptr_t)p, false, true, true); 
  if (result) {
    kprintf("Changed protections at %p, \nnumber of free pages should be the same %d\n", p, free_page_counter);
  } else {
    kprintf("vm_protect failed with an error, freepage num unchanged %d\n", free_page_counter); 
  }

  //    with unmapped value
  p = (int*)0x500430020; // same unmapped value used with vm_unmap test
  result = vm_protect(root, (uintptr_t)p, false, true, true); 
  if (result) {
    kprintf("Changed protections at %p, \nnumber of free pages should be the same %d\n", p, free_page_counter);
  } else {
    kprintf("vm_protect failed with an error, freepage num unchanged %d\n", free_page_counter); 
  }


	// We're done, just hang...
	halt();
}
