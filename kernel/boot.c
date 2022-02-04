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
  char* loc = str;  
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

uint64_t d_helper(uint64_t value) {
  uint64_t holder = value / 10; 
}

void kprint_d(uint64_t value){
  //if we are out of numbers just stop
  // if (value / 10 == 0) {
  //   return; 
  // } else {
  //   // if not, mod this, recurse (down by a power of 10)
  //   //   and do it again
  //   char num = value % 10; 
  //   kprint_d(value / 10);
  //   kprint_c(num); 
  // }

  uint64_t digits[20];  //max number of digits for number 2^64
  uint64_t place = 19; // keeps track of what index of array we're at (remember offset!)
  
  if(value == 0) {
    // deal if 0 
    return; 
  }

  // PROBLEM: OFF BY ONE ERROR somewhere, only impacts the "first digit"/last thing dealt with from the mod

  // fill array
  while(value / 10 != 0) {
    digits[place] = value % 10; 
    value = value / 10; 
    if(value < 10) { // if we get down to the last digit, just print it
      kprint_c(value + 48); 
    }
    place --; 
  }

  // print the array: 
  for(place; place < 20; place++) {
    kprint_c(digits[place] + 48); // 48 is value of 0 in ascii (aka offset is needed)
  }
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

  // test kprint_d
  kprint_d(2);
  //kprint_d(1000); 
  kprint_d(1342); 
   

  //test kprint_s
  kprint_s("hello");
  

	// We're done, just hang...
	halt();
}
