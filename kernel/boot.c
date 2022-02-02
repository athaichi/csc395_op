#include <stdint.h>
#include <stddef.h>

#include "stivale2.h"
#include "util.h"

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

// Find a tag with a given ID
void* find_tag(struct stivale2_struct* hdr, uint64_t id) {
  // Start at the first tag
	struct stivale2_tag* current = (struct stivale2_tag*)hdr->tags;

  // Loop as long as there are more tags to examine
	while (current != NULL) {
    // Does the current tag match?
		if (current->identifier == id) {
			return current;
		}

    // Move to the next tag
		current = (struct stivale2_tag*)current->next;
	}

  // No matching tag found
	return NULL;
}

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

// NEW STUFF // TODO ~~~~~~~
uint64_t kstrlen(const char* str) {
  uint64_t len = 0;
  const char* loc = str;  
  while(loc != NULL) {
    len++; 
    loc++; 
  }
  
  return len; 
}
//

void kprint_c(char c) {
  term_write(&c, 1);
}

void kprint_s(const char* str) {
  // ideally: 
  uint64_t len = kstrlen(str);
  term_write(str, len);
}

void kprint_d(uint64_t value){
  // TODO
}

void kprint_x(uint64_t value) {
  // TODO
}

void kprint_p(void* ptr) {
  // TODO
}

// END NEW STUFF ~~~~~~~

void _start(struct stivale2_struct* hdr) {
  // We've booted! Let's start processing tags passed to use from the bootloader
  term_setup(hdr);

  // Print a greeting
  term_write("Hello Kernel!\n", 14);
  term_write("this is a test\n", 15);

  //term_write("testing again\n");

  // test kprint_c
  char test = 'h';  
  kprint_c(test);
  test = 'H'; 
  kprint_c(test);
  kprint_c(' ');
  kprint_c('}');
  kprint_c('Q');
  kprint_c('q'); 
  kprint_c('\n');

  //test kprint_s
  kprint_s("hello");
  

	// We're done, just hang...
	halt();
}
