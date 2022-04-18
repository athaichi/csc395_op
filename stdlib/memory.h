#pragma once

#include <stdint.h>
#include <stdarg.h>

void kmemset(void *arr, uint32_t c, size_t len); 
void k_memcpy (void* dest, void* src, uint64_t size);