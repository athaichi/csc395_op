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
#include "executables.h"



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
  char_write(key); 

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
  idt_setup(); 

  // Keyboard stuff
  pic_init(); 
  pic_unmask_irq(1); 

  // set up customized handler functions: 
  // allow us to make system calls!
  idt_set_handler(0x80, syscall_entry, IDT_TYPE_TRAP);
  // allow us to handle keyboard interrupts
  idt_set_handler(IRQ1_INTERRUPT, keyboard_interrupt, IDT_TYPE_TRAP);

  // Print a greeting
  //term_write("Hello Kernel!\n", 14);

  //all_tests(); 

 // test read
  char buf[6];
  long rc = syscall(SYS_READ, 0, buf, 5);
  if (rc < 0) {
    kprintf("read failed\n");
  } else {
    buf[rc] = '\0';
    kprintf("read '%s'\n", buf);
  }

  // // // test write
  // char buf2[6] = "olleh"; 
  // long rc2 = syscall(SYS_WRITE, 1, buf2, 6); 
  // if (rc2 < 0) {
  //   kprintf("write failed\n"); 
  // } else {
  //   rc2 = syscall(SYS_READ, 0, buf2, 5); 
  //   buf2[rc2] = '\0';
  //   kprintf("wrote '%s'\n", buf2); 
  // }

  // get modules
  // kprint_s("Modules: \n"); 
  // find_modules(hdr); 

	// We're done, just hang...
	halt();
}
