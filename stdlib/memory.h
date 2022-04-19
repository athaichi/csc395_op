#pragma once

#include <stdint.h>
#include <stddef.h>

void kmemset(void *arr, uint32_t c, size_t len); 
void kmemcpy (void* dest, void* src, uint64_t size);