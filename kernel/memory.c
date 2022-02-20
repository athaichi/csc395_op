#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "memory.h"
#include "stivale2.h"
#include "kprint.h"

void usable_memory(struct stivale2_struct* hdr) {
  // print section label 
  kprint_s("Usable memory: \n"); 

  // find all the correct tags
  struct stivale2_struct_tag_kernel_base_address* base_address = find_tag(hdr, STIVALE2_STRUCT_TAG_KERNEL_BASE_ADDRESS_ID); 
  struct stivale2_struct_tag_memmap* memmap = find_tag(hdr, STIVALE2_STRUCT_TAG_MEMMAP_ID); 
  struct stivale2_struct_tag_hhdm* hhdm = find_tag(hdr, STIVALE2_STRUCT_TAG_HHDM_ID); 

  // iterate through all mapped memory 
  for(uint64_t current = 0; current < memmap->entries; current++) {
    struct stivale2_mmap_entry cur = memmap->memmap[current]; 
    
    // if section of memory is usable
    if(cur.type == 1) { 
      // print physical address 
      kprint_s("    ");
      kprint_d(cur.base); 
      kprint_c('-'); 
      kprint_d(cur.base + cur.length); 

      kprint_s(" is mapped to "); 

      // print virtual address
      kprint_d(cur.base - base_address->physical_base_address + hhdm->addr); 
      kprint_c('-'); 
      kprint_d(cur.base - base_address->physical_base_address + hhdm->addr + cur.length); 
      
      // format for next entry 
      kprint_c('\n');
    }
  }
}

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