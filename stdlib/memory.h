#pragma once

#include <stdint.h>
#include <stddef.h>

void memset(void *arr, uint32_t c, size_t len); 
void memcpy (void* dest, void* src, uint64_t size);
void * mmap(void *addr, size_t len, int prot, int flags, int fd, int offset);
void * malloc(size_t sz); 
