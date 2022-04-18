#include <stdint.h>
#include <stdarg.h>

// credit: https://aticleworld.com/memset-in-c/ 
void kmemset(void *arr, uint32_t c, size_t len) {
    uint8_t *current = arr; 
    for (size_t  i = 0; i < len; i++) {
        current[i] = c; 
    }

    return; 
}

// implementation from https://www.geeksforgeeks.org/write-memcpy/ 
void k_memcpy (void* dest, void* src, uint64_t size) {
    char* csrc = (char*)src; 
    char* cdest = (char*)dest; 

    // copy byte by byte
    for (int i = 0; i < size; i++) {
        //kprintf("copying from src to dest...");
        *(cdest++) = *(csrc++);  
        //kprintf("copied!\n"); 
    }
}