#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "kprint.h"
#include "memory.h"

void find_modules(struct stivale2_struct* hdr) {
  struct stivale2_struct_tag_modules* modules = find_tag(hdr, STIVALE2_STRUCT_TAG_MODULES_ID);

  for(uint64_t current = 0; current < modules->module_count; current++) {
    struct stivale2_module cur = modules->modules[current]; 
    
    //print all the modules
    //print name
    // for (int i = 0; i<128; i++) {
    //   kprint_c(cur.string[i]); 
    //   if (cur.string[i] == '\0') { break; }
    // }
    kprint_s("    0x");
    kprint_x(cur.begin); 
    kprint_s(" - 0x"); 
    kprint_x(cur.end); 

    // format for next entry 
    kprint_c('\n');
  }
}