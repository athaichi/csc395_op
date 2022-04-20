#include <stdint.h>
#include <stddef.h>

#include "../kernel/systemcalls.h"

// credit: https://aticleworld.com/memset-in-c/ 
void memset(void *arr, uint32_t c, size_t len) {
    uint8_t *current = arr; 
    for (size_t  i = 0; i < len; i++) {
        current[i] = c; 
    }

    return; 
}

// implementation from https://www.geeksforgeeks.org/write-memcpy/ 
void memcpy (void* dest, void* src, uint64_t size) {
    char* csrc = (char*)src; 
    char* cdest = (char*)dest; 

    // copy byte by byte
    for (int i = 0; i < size; i++) {
        //kprintf("copying from src to dest...");
        *(cdest++) = *(csrc++);  
        //kprintf("copied!\n"); 
    }
}

void * mmap(void *addr, size_t len, int prot, int flags, int fd, int offset) {
    void* map = (void*)syscall(SYS_MAP, addr, len, prot, flags, fd, offset); 
    return map; 
}

// Round a value x up to the next multiple of y
#define ROUND_UP(x, y) ((x) % (y) == 0 ? (x) : (x) + ((y) - (x) % (y)))

void* bump = NULL;
size_t space_remaining = 0;

void* malloc(size_t sz) {
  // Round sz up to a multiple of 16
  sz = ROUND_UP(sz, 16);

  // Do we have enough space to satisfy this allocation?
  if (space_remaining < sz) {
    // No. Get some more space using `mmap`
    size_t rounded_up = ROUND_UP(sz, PAGESIZE);
    void* newmem = mmap(NULL, rounded_up, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    // Check for errors
    if (newmem == NULL) {
      return NULL;
    }

    bump = newmem;
    space_remaining = rounded_up;
  }

  // Grab bytes from the beginning of our bump pointer region
  void* result = bump;
  bump += sz;
  space_remaining -= sz;

  return result;
}

void free(void* p) {
  // Do nothing
}
