#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <memory.h>

#include "stivale2.h"
#include "util.h"
#include "interrupts.h"
#include "kprint.h"

// Make an IDT
idt_entry_t idt[256];

/**
 * Set an interrupt handler for the given interrupt number.
 *
 * \param index The interrupt number to handle
 * \param fn    A pointer to the interrupt handler function
 * \param type  The type of interrupt handler being installed.
 *              Pass IDT_TYPE_INTERRUPT or IDT_TYPE_TRAP from above.
 */
void idt_set_handler(uint8_t index, void* fn, uint8_t type) {

    // set fn to be a uintptr (uint64) so we can do bit shifting
    uintptr_t function = (uintptr_t)fn; 

    // offsets are the different parts of the handler function
    // bytes     0 - 1 - 2 - 3 - 4 - 5 - 6 - 7 -
    //           x x x x x x x x x x x x x x x x  u64 in hex
    // offset 0: x x x x 0 0 0 0 0 0 0 0 0 0 0 0  
    // offset 1: 0 0 0 0 x x x x 0 0 0 0 0 0 0 0  
    // offset 2: 0 0 0 0 0 0 0 0 x x x x x x x x  
    // uint16_t offset0 = (function >> 48) & 0xFF; 
    // uint16_t offset1 = (function >> 32) & 0x00FF;
    // uint32_t offset2 = function & 0xFFFF;

    // bytes     0 - 1 - 2 - 3 - 4 - 5 - 6 - 7 -
    //           x x x x x x x x x x x x x x x x  u64 in hex
    // offset 0: 0 0 0 0 0 0 0 0 0 0 0 0 x x x x 
    // offset 1: 0 0 0 0 0 0 0 0 x x x x 0 0 0 0  
    // offset 2: x x x x x x x x 0 0 0 0 0 0 0 0  
    
    uint16_t offset0 = function & 0xFFFF; 
    uint16_t offset1 = (function >> 16) & 0xFFFF;
    uint32_t offset2 = (function >> 32) & 0xFFFFFFFF;

    idt[index].offset_0 = offset0; 
    idt[index].selector = KERNEL_CODE_SELECT;  // run in kernel mode, set to KERNEL_CODE_SELECT for user mode 
    idt[index].ist = 0;        // aren't using an interrupt stack table, so just pass 0
    idt[index]._unused_0 = 0; 
    idt[index].type = type;    // using the parameter passed to this function
    idt[index]._unused_1 = 0;  
    idt[index].dpl = 3;        // run the handler in kernel mode, change to 3 for user mode
    idt[index].present = 1;    // the entry is present 
    idt[index].offset_1 = offset1; 
    idt[index].offset_2 = offset2;  
    idt[index]._unused_2 = 0;  
}

// This struct is used to load an IDT once we've set it up
typedef struct idt_record {
  uint16_t size;
  void* base;
} __attribute__((packed)) idt_record_t;

// Create the handler functions
__attribute__((interrupt))
void handler0(interrupt_context_t* ctx) {
  kprintf("Debug Exception\n");
  halt();
}

__attribute__((interrupt))
void handler1(interrupt_context_t* ctx) {
  kprintf("NMI Interrupt\n");
  halt();
}

__attribute__((interrupt))
void handler2(interrupt_context_t* ctx) {
  kprintf("Breakpoint\n");
  halt();
}

__attribute__((interrupt))
void handler3(interrupt_context_t* ctx) {
  kprintf("Overflow\n");
  halt();
}

__attribute__((interrupt))
void handler4(interrupt_context_t* ctx) {
  kprintf("BOUND Range Exceeded\n");
  halt();
}

__attribute__((interrupt))
void handler5(interrupt_context_t* ctx) {
  kprintf("Invalid Opcode (Undefined Opcode)\n");
  halt();
}

__attribute__((interrupt))
void handler6(interrupt_context_t* ctx) {
  kprintf("Device Not Available (No Math Coprocessor)\n");
  halt();
}

__attribute__((interrupt))
void handler7(interrupt_context_t* ctx, uint64_t ec) {
  kprintf("Double Fault (error code = %d)\n", ec);
  halt();
}

__attribute__((interrupt))
void handler8(interrupt_context_t* ctx) {
  kprintf("Coprocessor Segment Overrun (reserved)\n");
  halt();
}

__attribute__((interrupt))
void handler9(interrupt_context_t* ctx, uint64_t ec) {
  kprintf("Invalid TSS (error code = %d)\n", ec);
  halt();
}

__attribute__((interrupt))
void handler10(interrupt_context_t* ctx, uint64_t ec) {
  kprintf("Segment Not Present (error code = %d)\n", ec);
  halt();
}

__attribute__((interrupt))
void handler11(interrupt_context_t* ctx, uint64_t ec) {
  kprintf("Stack-Segment Fault (error code = %d)\n", ec);
  halt();
}

__attribute__((interrupt))
void handler12(interrupt_context_t* ctx, uint64_t ec) {
  kprintf("General Protection (error code = %d)\n", ec);
  halt();
}

__attribute__((interrupt))
void handler13(interrupt_context_t* ctx, uint64_t ec) {
  kprintf("Page Fault (error code = %d)\n", ec);
  halt();
}

__attribute__((interrupt))
void handler14(interrupt_context_t* ctx) {
  kprintf("(Intel reserved. Do not use)\n");
  halt();
}

__attribute__((interrupt))
void handler15(interrupt_context_t* ctx) {
  kprintf("x87 FPU Floating Point Error (Math Fault)\n");
  halt();
}

__attribute__((interrupt))
void handler16(interrupt_context_t* ctx, uint64_t ec) {
  kprintf("Alignment Check (error code = %d)\n", ec);
  halt();
}

__attribute__((interrupt))
void handler17(interrupt_context_t* ctx) {
  kprintf("Machine Check\n");
  halt();
}

__attribute__((interrupt))
void handler18(interrupt_context_t* ctx) {
  kprintf("SIMD Floating Point Exception\n");
  halt();
}

__attribute__((interrupt))
void handler19(interrupt_context_t* ctx) {
  kprintf("Virtualization Exception\n");
  halt();
}

__attribute__((interrupt))
void handler20(interrupt_context_t* ctx, uint64_t ec) {
  kprintf("Control Protection Exception (error code = %d)\n", ec);
  halt();
}

/**
 * Initialize an interrupt descriptor table, set handlers for standard exceptions, and install
 * the IDT.
 */
void idt_setup() {
  // Step 1: Zero out the IDT, probably using memset (which you'll have to implement)
  memset(idt, 0, sizeof(idt)); 

  // -----------------------------------------------

  // Step 2: Use idt_set_handler() to set handlers for the standard exceptions (1--21)
  // TRAPS, and FAULTS can be continued if fixed, 
  // ABORTS cannot be continued
  uint8_t indices[21] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21}; 
  
  // Set each individual handler
  // for now, interrupts use TYPE_INTERRUPT, and everything else is a TYPE_TRAP
  idt_set_handler(indices[0], handler0, IDT_TYPE_TRAP);       // fault/trap
  idt_set_handler(indices[1], handler1, IDT_TYPE_INTERRUPT);  // interrupt
  idt_set_handler(indices[2], handler2, IDT_TYPE_TRAP);       // trap
  idt_set_handler(indices[3], handler3, IDT_TYPE_TRAP);       // trap
  idt_set_handler(indices[4], handler4, IDT_TYPE_TRAP);       // fault
  idt_set_handler(indices[5], handler5, IDT_TYPE_TRAP);       // fault
  idt_set_handler(indices[6], handler6, IDT_TYPE_TRAP);       // fault
  idt_set_handler(indices[7], handler7, IDT_TYPE_TRAP);       // abort
  idt_set_handler(indices[8], handler8, IDT_TYPE_TRAP);       // fault
  idt_set_handler(indices[9], handler9, IDT_TYPE_TRAP);       // fault
  idt_set_handler(indices[10], handler10, IDT_TYPE_TRAP);      // fault
  idt_set_handler(indices[11], handler11, IDT_TYPE_TRAP);      // fault
  idt_set_handler(indices[12], handler12, IDT_TYPE_TRAP);      // fault
  idt_set_handler(indices[13], handler13, IDT_TYPE_TRAP);      // fault 
  idt_set_handler(indices[14], handler14, IDT_TYPE_TRAP);      // fault 
  idt_set_handler(indices[15], handler15, IDT_TYPE_TRAP);      // fault 
  idt_set_handler(indices[16], handler16, IDT_TYPE_TRAP);      // fault 
  idt_set_handler(indices[17], handler17, IDT_TYPE_TRAP);      // abort 
  idt_set_handler(indices[18], handler18, IDT_TYPE_TRAP);      // fault 
  idt_set_handler(indices[19], handler19, IDT_TYPE_TRAP);      // fault 
  idt_set_handler(indices[20], handler20, IDT_TYPE_TRAP);      // fault 
 
  
  // -------------------------------------------------
  
  // Step 3: Install the IDT
  idt_record_t record = {
    .size = sizeof(idt),
    .base = idt
  };
  __asm__("lidt %0" :: "m"(record));
}

