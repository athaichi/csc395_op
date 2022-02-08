# Starter Kernel
This is the starter kernel code for the spring 2022 offering of CSC 395: Advanced Operating Systems at Grinnell College. This project includes the basic elements required to build and run a simple kernel on an x86_64 machine emulated with QEMU.

Specifically, this starter code comes from Professor Charlie Curtsinger. 

## Acknowledgements
This starter code is based on the following example projects:
- [OSDev.org Bare Bones Kernel](https://wiki.osdev.org/Bare_bones)
- [Stivale2 Barebones Kernel](https://github.com/stivale/stivale2-barebones)

In addition to the above example projects, the following references were used when setting up this code:
- [OSDev.org GCC Cross Compiler Instructions](https://wiki.osdev.org/GCC_Cross-Compiler)
- [Stivale2 Reference](https://github.com/stivale/stivale/blob/master/STIVALE2.md)


## Code Organization
This is an ongoing project. Be prepared for code and organization of said code to change dramatically. Below I have detailed the each implemented function, separated by which file it is. Once I have header files implemented, general function descriptions will be listed there above their specific function. Function-specific implementation comments are listed with the actual implementation itself. 

All functions are implemented from descriptions given in class. To see full assignment instructions see the [class website](https://curtsinger.cs.grinnell.edu/teaching/2022S/CSC395/).

boot.c
-------
- `void kprint_c(char c)`: print a single character to the terminal
- `void kprint_s(const char *)`: print a string to the terminal
- `void kprint_d(uint64_t value)`: print a u64 bit integer to the terminal in decimal
- `void kprint_x(uint64_t value)`: print a u64 bit integer to terminal in hex (lowercase)
- `void kprint_p(void* ptr)`: print the value of a pointer to the terminal in hex with leading 0x. 
- `void kprintf(const char* format, ...)`: print a formatted string to the terminal

- `void usable_memory(struct stivale2_struct* hdr)`: prints all usable memory to the terminal
    - First interval is the range of physical memory, second interval is the correspondingly mapped virtual memory

