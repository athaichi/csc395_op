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
}

// ==============================================
// ==============================================
// // ===============================================
// // ===============================================


// global! keep track of number of free pages - just for testing purposes
extern int free_page_counter;

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
  // char buf[6] = "hello";
  // long rc = syscall(SYS_READ, 0, buf, 5);
  // if (rc < 0) {
  //   kprintf("read failed\n");
  // } else {
  //   buf[rc] = '\0';
  //   kprintf("read '%s'\n", buf);
  // }

  // // test write
  // char buf2[6]; 
  // long rc2 = syscall(SYS_WRITE, 'h', buf2, 1); 
  // if (rc2 < 0) {
  //   kprintf("write failed\n"); 
  // } else {
  //   rc = syscall(SYS_READ, 0, buf2, 5); 
  //   buf2[rc] = '\0';
  //   kprintf("wrote '%s'\n", buf2); 
  // }


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
