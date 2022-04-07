#pragma once
#include <stdbool.h>
#include "stivale2.h"

uintptr_t read_cr3();
uintptr_t get_hhdm(struct stivale2_struct* hdr);
void translate(void* address, struct stivale2_struct* hdr);

uintptr_t pmem_alloc(); 
void pmem_free(uintptr_t p); 

void mem_init(struct stivale2_struct* hdr); 
bool vm_map(uintptr_t root, uintptr_t address, bool usable, bool writable, bool executable); 
bool vm_unmap(uintptr_t root, uintptr_t address); 
bool vm_protect(uintptr_t root, uintptr_t address, bool user, bool writable, bool executable); 



