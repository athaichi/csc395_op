#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "stivale2.h"
#include "util.h"
#include "interrupts.h"
#include "page.h"
#include "kprint.h"
#include "memory.h"

// Reserve space for the stack
static uint8_t stack[8192];

// Request a terminal from the bootloader
static struct stivale2_header_tag_terminal terminal_hdr_tag = {
	.tag = {
    .identifier = STIVALE2_HEADER_TAG_TERMINAL_ID,
    .next = 0
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

// for testing interrupts
// static struct stivale2_tag unmap_null_hdr_tag = {
//   .identifier = STIVALE2_HEADER_TAG_UNMAP_NULL_ID,
//   .next = 0
// };

// -------------------------------------------------

void _start(struct stivale2_struct* hdr) {
  // We've booted! Let's start processing tags passed to use from the bootloader
  term_setup(hdr);
  idt_setup(); 

  // Print a greeting
  term_write("Hello Kernel!\n", 14);
  term_write("this is a test\n", 15);

  // test kprint_c
  // char test = 'h';  
  // kprint_c(test);
  // test = 'H'; 
  // kprint_c(test);
  // kprint_c(' ');
  // kprint_c('}');
  // kprint_c('Q');
  // kprint_c('q'); 
  // kprint_c('\n');

  // test kprint_d
  // kprint_d(2);
  // kprint_c('\n'); 
  // kprint_d(0); 
  // kprint_c('\n'); 
  // kprint_d(1000); 
  // kprint_c('\n'); 
  // kprint_d(1342); 
  // kprint_c('\n'); 
  // kprint_d(5379); 
   

  //test kprint_s
  // kprint_s("hello\n");
  // kprint_s("hi! nice to meet you????\n"); 
  // kprint_s("4384>?<P{_)(*&^$#@!\t check tabs"); 

  // test strlen
  // kprint_d(kstrlen("hello")); // should be 5
  // kprint_c(' ');  
  // kprint_d(kstrlen("hi")); // should be 2
  // kprint_c(' '); 
  // kprint_d(kstrlen("hey this is joe")); // should be 15

  // test kprint_x
  // kprint_x(0); // should be 0
  // kprint_c(' '); 
  // kprint_x(10); // should be a
  // kprint_c(' ');
  // kprint_x(48362); // should be bcea
  // kprint_c(' ');
  // kprint_x(4738295); // should be 484cf7

  // test kprint_p
    kprint_p(stack); 

  // test usable_memory
  //usable_memory(hdr); 

  // test idt
  // int* p = (int*)0x1;
  // *p = 123; 
  //__asm__("int $2");

  // test paging
  translate(read_cr3(), _start); 

	// We're done, just hang...
	halt();
}
