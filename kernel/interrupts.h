#pragma once

// void idt_set_handler(uint8_t index, void* fn, uint8_t type);
// void idt_setup();
// void k_memset(void *arr, uint32_t c, size_t len); 

// // Every interrupt handler must specify a code selector. We'll use entry 5 (5*8=0x28), which
// // is where our bootloader set up a usable code selector for 64-bit mode.
// #define IDT_CODE_SELECTOR 0x28

// // IDT entry types
// #define IDT_TYPE_INTERRUPT 0xE
// #define IDT_TYPE_TRAP 0xF