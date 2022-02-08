#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

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

// credit: https://stackoverflow.com/questions/3213827/how-to-iterate-over-a-string-in-c 
uint64_t kstrlen(const char* str) {
  uint64_t len = 0;
  while(*str) { //if we haven't finished iterating over the string, 
    len++; // increase length and 
    str++; // check next charactter
  }
  return len; 
}
//

void kprint_c(char c) {
  term_write(&c, 1);
}

void kprint_s(const char* str) {
  uint64_t len = kstrlen(str);
  term_write(str, len);
}

void kprint_d(uint64_t value){
  if(value < 10) {
    kprint_c(value + 48);
    return; 
  }

  uint64_t digits[20];  //max number of digits for number 2^64
  uint64_t place = 19; // keeps track of what index of array we're at (remember offset!)
  
  // fill array
  while(value / 10 != 0) {
    digits[place] = value % 10; 
    value = value / 10; 
    if(value < 10) { // if we get down to the last digit, just print it
      kprint_c(value + 48); 
      place++; // fix place to be the previously entered digit
    }
    place --; 
  }

  // print the array: 
  for(place; place < 20; place++) {
    kprint_c(digits[place] + 48); // 48 is value of 0 in ascii (aka offset is needed)
  }
}

void kprint_x(uint64_t value) {
  if(value < 17) {
    if(value < 10) { kprint_c(value + 48); }
    else { kprint_c(value + 97 - 10); } // add 97 to get to 'a', -10 to find offset within a-f
    return; 
  }

  uint64_t digits[16];  //max number of digits for number 2^64 represented in hex
  uint64_t place = 15; // keeps track of what index of array we're at (remember offset!)
  
  // fill array
  while(value / 16 != 0) {
    digits[place] = value % 16; 
    value = value / 16; 
    if(value < 16) { // if we get down to the last digit, just print it
      if(value < 10) { kprint_c(value + 48); }
      else{ kprint_c(value + 97 - 10); } // add 97 to get to 'a', -10 to find offset within a-f
      place++; // fix place to be the previously entered digit
    }
    place --; 
  }

  // print the array: 
  for(place; place < 16; place++) {
    if (digits[place] < 10) { kprint_c(digits[place] + 48); } // 48 is value of 0 in ascii (aka offset is needed)
    else { kprint_c(digits[place] + 97 - 10); } // add 97 to get to 'a', -10 to find offset within a-f
  }
}

void kprint_p(void* ptr) {
  kprint_c('0'); 
  kprint_c('x'); 
  kprint_x(ptr); 
  return; 
}

// This is taken directly from the class website after completing the implementation 
//   together as a class 
void kprintf(const char* format, ...) {
  // Start processing variadic arguments
  va_list args;
  va_start(args, format);

  // Loop until we reach the end of the format string
  size_t index = 0;
  while (format[index] != '\0') {
    // Is the current charater a '%'?
    if (format[index] == '%') {
      // Yes, print the argument
      index++;
      switch(format[index]) {
        case '%':
          kprint_c('%');
          break;
        case 'c':
          kprint_c(va_arg(args, int));
          break;
        case 's':
          kprint_s(va_arg(args, char*));
          break;
        case 'd':
          kprint_d(va_arg(args, uint64_t));
          break;
        case 'x':
          kprint_x(va_arg(args, int64_t));
          break;
        case 'p':
          kprint_p(va_arg(args, void*));
          break;
        default:
          kprint_s("<not supported>");
      }
    } else {
      // No, just a normal character. Print it.
      kprint_c(format[index]);
    }
    index++;
  }

  // Finish handling variadic arguments
  va_end(args);
}

void usable_memory(struct stivale2_struct* hdr) {
  kprint_s("Usable memory: \n    "); 
  struct stivale2_struct_tag_kernel_base_address* base_address = find_tag(hdr, STIVALE2_STRUCT_TAG_KERNEL_BASE_ADDRESS_ID); 
  struct stivale2_struct_tag_memmap* memmap = find_tag(hdr, STIVALE2_STRUCT_TAG_MEMMAP_ID); 
  struct stivale2_struct_tag_hhdm* hhdm = find_tag(hdr, STIVALE2_STRUCT_TAG_HHDM_ID); 

  for(uint64_t current = 0; current < memmap->entries; current++) {
    struct stivale2_mmap_entry cur = memmap->memmap[current]; 
    if(cur.type == 1) { // section of memory is usable

      // print physical address 
      kprint_p(cur.base); 
      kprint_c('-'); 
      kprint_p(cur.base + cur.length); 

      kprint_s(" is mapped to "); 

      kprint_p(cur.base - base_address->physical_base_address + hhdm->addr); 
      kprint_c('-'); 
      kprint_p(cur.base - base_address->physical_base_address + hhdm->addr + cur.length); 
      kprint_c('\n');
      kprint_s("    ");
    }
  }
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
  //  kprint_p(stack); 

  // test usable_memory
  usable_memory(hdr); 

	// We're done, just hang...
	halt();
}
