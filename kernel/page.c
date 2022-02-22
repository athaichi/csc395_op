// #include <stdint.h>
// #include <stdbool.h>

// #include "stivale2.h"

// // note to self: bottom bits are on the right

// // create a page entry
// // should be 64 bits
// typedef struct page_table_entry {
//   bool present : 1;
//   bool writable : 1;
//   bool user : 1;      // also known as usable
//   uint16_t unused : 9;
//   uint64_t address : 51;
//   bool no_execute : 1;
// } __attribute__((packed)) pt_entry_t;

// // get cr3 register
// uintptr_t read_cr3() {
//   uintptr_t value;
//   __asm__("mov %%cr3, %0" : "=r" (value));
//   return value;
// }

// // create a helper to get physical to virtual 
// void* get_hhdm(struct stivale2_struct* hdr) {
//   struct stivale2_struct_tag_hhdm* hhdm = find_tag(hdr, STIVALE2_STRUCT_TAG_HHDM_ID);
// }

// /**
//  * Translate a virtual address to its mapped physical address
//  *
//  * \param address     The virtual address to translate
//  */
// void translate(void* address, struct stivale2_struct* hdr) {

//   // find starting table address
//   uintptr_t table_phys = read_cr3() & 0xFFFFFFFFFFFFF000;

//   // get hhdm tag 
//   uintptr_t hhdm_base = (uintptr_t)(get_hhdm(hdr)); 

//   // find the first table 
//   pt_entry_t * table = (pt_entry_t *)(table_phys + hhdm_base); 

//   // make parameter actually usable
//   uint64_t linear_addr = (uint64_t)address; 

//   // separate linear address into index pieces
//   uint16_t indices[] = {
//     linear_addr & 0xFFF,         // offset
//     (linear_addr >> 12) & 0x1FF, // level 1
//     (linear_addr >> 21) & 0x1FF, // level 2
//     (linear_addr >> 30) & 0x1FF, // level 3
//     (linear_addr >> 39) & 0x1FF, // level 4
//   };


//   kprintf("og address: %p", address);
//   for(int i = 4; i >= 0 ; i--) {
//     kprintf("\n   level %d index is: %p", i, indices[i]); 
//   }
//   kprint_c('\n'); 
  
//   // Begin terminal output
//   kprintf("Translating %p \n", address); 

//   for (int i = 4; i > 0; i--) {
//     uint16_t index = indices[i]; 
//     if (table[index].present) {
//       kprintf("   level %d : %s", i, table[index].user ? "user" : "kernel"); 
//       kprintf(" %s", table[index].writable ? "writable" : ""); 
//       kprintf(" %s", table[index].no_execute ? "" : "executable"); 

//       // Get physical address of the next level table
//       table_phys = table[index].address << 12; 
//       table = (pt_entry_t*)(table_phys + hhdm_base); 

//       kprintf(" ->  %p", table_phys); 

//     } else {
//       kprintf("   Not Present!\n"); 
//       return; 
//     }
//   }

//   // get final offset 
//   uintptr_t result = table_phys + indices[0]; 

//   // print final offset address 
//   kprintf("%p maps to %p\n", address, result); 

//   return; 
// }
