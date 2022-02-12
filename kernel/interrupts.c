#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "stivale2.h"
#include "util.h"
#include "interrupts.h"

// A struct the matches the layout of an IDT entry
typedef struct idt_entry {
  uint16_t offset_0;
  uint16_t selector;
  uint8_t ist : 3;
  uint8_t _unused_0 : 5;
  uint8_t type : 4;
  uint8_t _unused_1 : 1;
  uint8_t dpl : 2;
  uint8_t present : 1;
  uint16_t offset_1;
  uint32_t offset_2;
  uint32_t _unused_2;
} __attribute__((packed)) idt_entry_t;

typedef struct interrupt_context {
  uintptr_t ip;
  uint64_t cs;
  uint64_t flags;
  uintptr_t sp;
  uint64_t ss;
} __attribute__((packed)) interrupt_context_t;

__attribute__((interrupt))
void handler0(interrupt_context_t* ctx) {
  kprintf("Divide Error\n");
  halt();
}

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
   
    // some helpful values for manipulating 
    uint32_t zero32 = 0; 
    uint8_t zero8 = 0; 
    uint8_t one8 = 1; 

    // set fn to be a uintptr (uint64) so we can do bit shifting
    uintptr_t function = (uintptr_t)fn; 

    // offsets are the different parts of the handler function
    // bytes     0 - 1 - 2 - 3 - 4 - 5 - 6 - 7 -
    //           x x x x x x x x x x x x x x x x  u64 in hex
    // offset 0: x x x x 0 0 0 0 0 0 0 0 0 0 0 0  
    // offset 1: 0 0 0 0 x x x x 0 0 0 0 0 0 0 0  
    // offset 2: 0 0 0 0 0 0 0 0 x x x x x x x x  
    uint16_t offset0 = (uint16_t)(function >> 48); // get bytes 0 1
    uint16_t offset1 = (uint16_t)((function << 16) >> 48); // get bytes 2 3
    uint32_t offset2 = (uint32_t)((function << 32) >> 32); // get bytes 4-7

    idt[index].offset_0 = offset0; 
    idt[index].selector = IDT_CODE_SELECTOR;  
    idt[index].ist = zero8;    // we aren't using an interrupt stack table, so just pass 0
    idt[index]._unused_0 = zero8;
    idt[index].type = type;    // using the parameter passed to this function
    idt[index]._unused_1 = zero8; 
    idt[index].dpl = zero8;    // run the handler in kernel mode
    idt[index].present = one8; // the entry is present 
    idt[index].offset_1 = offset1; 
    idt[index].offset_2 = offset2;  
    idt[index]._unused_2 = zero32; 
}

// This struct is used to load an IDT once we've set it up
typedef struct idt_record {
  uint16_t size;
  void* base;
} __attribute__((packed)) idt_record_t;

/**
 * Initialize an interrupt descriptor table, set handlers for standard exceptions, and install
 * the IDT.
 */
void idt_setup() {
  // Step 1: Zero out the IDT, probably using memset (which you'll have to implement)
  // Write me!

  // -----------------------------------------------

  // Step 2: Use idt_set_handler() to set handlers for the standard exceptions (1--21)
  // TRAPS, and FAULTS can be continued if fixed, 
  // ABORTS cannot be continued
  uint8_t indices[21] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21}; 
  
  // Set each individual handler
  // for now everything is just using a dummy handler function
  // for now, interrupts use TYPE_INTERRUPT, and everything else is a TYPE_TRAP
  idt_set_handler(indices[0], handler0, IDT_TYPE_TRAP);       // fault/trap
  idt_set_handler(indices[1], handler0, IDT_TYPE_INTERRUPT);  // interrupt
  idt_set_handler(indices[2], handler0, IDT_TYPE_TRAP);       // trap
  idt_set_handler(indices[3], handler0, IDT_TYPE_TRAP);       // trap
  idt_set_handler(indices[4], handler0, IDT_TYPE_TRAP);       // fault
  idt_set_handler(indices[5], handler0, IDT_TYPE_TRAP);       // fault
  idt_set_handler(indices[6], handler0, IDT_TYPE_TRAP);       // fault
  idt_set_handler(indices[7], handler0, IDT_TYPE_TRAP);       // abort
  idt_set_handler(indices[8], handler0, IDT_TYPE_TRAP);       // fault
  idt_set_handler(indices[9], handler0, IDT_TYPE_TRAP);       // fault
  idt_set_handler(indices[10], handler0, IDT_TYPE_TRAP);      // fault
  idt_set_handler(indices[11], handler0, IDT_TYPE_TRAP);      // fault
  idt_set_handler(indices[12], handler0, IDT_TYPE_TRAP);      // fault
  idt_set_handler(indices[13], handler0, IDT_TYPE_TRAP);      // fault 
  idt_set_handler(indices[14], handler0, IDT_TYPE_TRAP);      // fault 
  idt_set_handler(indices[15], handler0, IDT_TYPE_TRAP);      // fault 
  idt_set_handler(indices[16], handler0, IDT_TYPE_TRAP);      // fault 
  idt_set_handler(indices[17], handler0, IDT_TYPE_TRAP);      // abort 
  idt_set_handler(indices[18], handler0, IDT_TYPE_TRAP);      // fault 
  idt_set_handler(indices[19], handler0, IDT_TYPE_TRAP);      // fault 
  idt_set_handler(indices[20], handler0, IDT_TYPE_TRAP);      // fault 
  
  // -------------------------------------------------
  
  // Step 3: Install the IDT
  idt_record_t record = {
    .size = sizeof(idt),
    .base = idt
  };
  __asm__("lidt %0" :: "m"(record));
}