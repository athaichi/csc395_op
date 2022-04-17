#pragma once

#include <stddef.h>
#include "stivale2.h"


void usable_memory(struct stivale2_struct* hdr);  
void* find_tag(struct stivale2_struct* hdr, uint64_t id); 
void kmemset(void *arr, uint32_t c, size_t len);