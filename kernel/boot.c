#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "stivale2.h"
#include "util.h"
#include "interrupts.h"
#include "page.h"
#include "kprint.h"
#include "memory.h"
#include "pic.h"
#include "port.h"

// Define read/write syscalls!
#define SYS_READ 0
#define SYS_WRITE 1

// Reserve space for the stack
static uint8_t stack[8192];

// Request a terminal from the bootloader
static struct stivale2_header_tag_terminal terminal_hdr_tag = {
	.tag = {
    .identifier = STIVALE2_HEADER_TAG_TERMINAL_ID,
    .next = 0
  },
  .flags = 0
};

// Declare the header for the bootloader
__attribute__((section(".stivale2hdr"), used))
static struct stivale2_header stivale_hdr = {
  // Use ELF file's default entry point
  .entry_point = 0,

  // Use stack (starting at the top)
  .stack = (uintptr_t)stack + sizeof(stack),

  // Bit 1: request pointers in the higher half
  // Bit 2: enable protected memory ranges (specified in PHDR)
  // Bit 3: virtual kernel mappings (no constraints on physical memory)
  // Bit 4: required
  .flags = 0x1E,
  
  // First tag struct
  .tags = (uintptr_t)&terminal_hdr_tag
};

typedef void (*term_write_t)(const char*, size_t);
term_write_t term_write = NULL;

void term_setup(struct stivale2_struct* hdr) {
  // Look for a terminal tag
  struct stivale2_struct_tag_terminal* tag = find_tag(hdr, STIVALE2_STRUCT_TAG_TERMINAL_ID);

  // Make sure we find a terminal tag
  if (tag == NULL) halt();

  // Save the term_write function pointer
	term_write = (term_write_t)tag->term_write;
}

// --------------------------------------------------

// =======================================================
// =======================================================

// Every interrupt handler must specify a code selector. We'll use entry 5 (5*8=0x28), which
// is where our bootloader set up a usable code selector for 64-bit mode.
#define IDT_CODE_SELECTOR 0x28

// IDT entry types
#define IDT_TYPE_INTERRUPT 0xE
#define IDT_TYPE_TRAP 0xF

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
    idt[index].selector = IDT_CODE_SELECTOR;  
    idt[index].ist = 0;        // aren't using an interrupt stack table, so just pass 0
    idt[index]._unused_0 = 0; 
    idt[index].type = type;    // using the parameter passed to this function
    idt[index]._unused_1 = 0;  
    idt[index].dpl = 0;        // run the handler in kernel mode
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

// create an array of keys following scan code from 
//   https://wiki.osdev.org/PS2_Keyboard#Scan_Code_Set_1
//  keys not easily delt with recieve '!' as a placeholder
uint8_t keys[] = {"!1234567890-=!!qwertyuiop[]!!asdfghjkl;'`!!zxcvbnm,./!*!!!"};
  // esc bksp tab enter, lctrl, lshift, fwdslash rshift, lalt, sp, caploc, end at caps loc (0x3A)

// take in a code and print corresponding key
char getkey(uint8_t code) {

  // Handle fancy characters
  //========================
  // esc
  if (code == 0x01) { return '\0'; }
  
  // backspace
  if (code == 0x0E) { return '\0'; }

  // tab
  if (code == 0x0F) { return '\t'; }

  // enter 
  if (code == 0x1C) { return '\n'; }

  // left control
  if (code == 0x1D) { return '\0'; }

  // left/right shift
  if ((code == 0x2A) || (code == 0x36)) { return '\0'; }

  // forward slash
  if (code == 0x2B) { return '\\'; }

  // left alt
  if (code == 0x38) { return '\0'; }

  // space
  if (code == 0x39) { return ' '; }

  // caps loc
  if (code == 0x3A) { return '\0'; }
  
  //keyup codes
  if (code >= 0x81) { return '\0'; }

  // Handle everything else
  // ======================
  return keys[code - 1]; 
}

// circular buffer and read/write pointers
char buffer[100]; 
volatile int8_t buffer_length = 0; // number of characters currently in the buffer
int8_t reading_index = 0; 
int8_t writing_index = 0; 

char char_read() {
  // get character off of buffer
  char returned = buffer[reading_index]; 

  // move the pointer to the next character to read and decrease buffer length
  reading_index++; 
  buffer_length--; 

  // check if we need to loop back around to the beginning of the array
  if (reading_index >= 100) { 
    reading_index = 0; 
  }

  return returned; 
}

void char_write(char key) {
  // add it to the buffer
  buffer[writing_index] = key; 
  writing_index++; 
  buffer_length++; 

  // if we reach the end of the index, loop back to the beginning
  if (writing_index >= 100) {
    writing_index = 0; 
  }

}

char read(int filedescriptor) {
  // validate file descriptor
  if (filedescriptor != 0) { return -1; }
  char_read(); 
  return 0; 
}

char write(int filedescriptor, char key) {
  if ((filedescriptor != 1) || (filedescriptor != 2)) { return -1; }
  char_write(key);
  return 0; 
}



// interrupt for key presses
__attribute__((interrupt))
void keyboard_interrupt(interrupt_context_t* ctx) {
  // get the keycode
  uint8_t code = inb(0x60);

  // make it the actual character
  char key = getkey(code);  

  // write character to buffer
  char_write(key); 

  outb(PIC1_COMMAND, PIC_EOI); 
}

/**
 * Read one character from the keyboard buffer. If the keyboard buffer is empty this function will
 * block until a key is pressed.
 *
 * \returns the next character input from the keyboard
 */
char kgetc() {

  // hang around until something enters the buffer
  while (buffer_length == 0) {
    //kprint_c('0'); 
  }

  char returned = char_read(); 

  //kprintf("interrput has happened and we're back in kgetc()...")
  //kprintf("about to hit the return\n");
  
  return returned; 
}

// credit: https://aticleworld.com/memset-in-c/ 
void k_memset(void *arr, uint32_t c, size_t len) {
    uint8_t *current = arr; 
    for (size_t  i = 0; i < len; i++) {
        current[i] = c; 
    }

    return; 
}


/**
 * Initialize an interrupt descriptor table, set handlers for standard exceptions, and install
 * the IDT.
 */
void idt_setup() {
  // Step 1: Zero out the IDT, probably using memset (which you'll have to implement)
  k_memset(idt, 0, sizeof(idt)); 

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

  idt_set_handler(IRQ1_INTERRUPT, keyboard_interrupt, IDT_TYPE_TRAP); 
  
  // -------------------------------------------------
  
  // Step 3: Install the IDT
  idt_record_t record = {
    .size = sizeof(idt),
    .base = idt
  };
  __asm__("lidt %0" :: "m"(record));
}

// for testing interrupts
static struct stivale2_tag unmap_null_hdr_tag = {
  .identifier = STIVALE2_HEADER_TAG_UNMAP_NULL_ID,
  .next = (uintptr_t)&unmap_null_hdr_tag
};

//----------------------------------------------------------
// SYSCALL STUFF:
int64_t syscall_handler(uint64_t nr, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
  kprintf("syscall %d: %d, %d, %d, %d, %d, %d\n", nr, arg0, arg1, arg2, arg3, arg4, arg5);
  
  // if we are reading, call the read function
  if (nr == SYS_READ) {char_read(); }

  // if we are writing, call the write function
  if (nr == SYS_WRITE) {char_write(arg0); }

  return 123;
}



extern int64_t syscall(uint64_t nr, ...);
extern void syscall_entry();
 

// -------------------------------------------------

void _start(struct stivale2_struct* hdr) {
  // We've booted! Let's start processing tags passed to use from the bootloader
  term_setup(hdr);
  //idt_setup(); 
  // Keyboard stuff
  pic_init(); 
  pic_unmask_irq(1); 

  idt_set_handler(0x80, syscall_entry, IDT_TYPE_TRAP);

  // Print a greeting
  term_write("Hello Kernel!\n", 14);
  term_write("this is a test\n", 15);

  all_tests(); 
   // Keyboard stuff
  pic_init(); 
  pic_unmask_irq(1); 

  // test usable_memory
  //usable_memory(hdr); 

  // test idt
  // int* p = (int*)0x1;
  // *p = 123; 
  //__asm__("int $2");

  // test paging
  //translate(read_cr3(), _start); 

  // test kgetc()
  // while(1) {
  //   kprintf("%c", kgetc()); 
  // }

  // test read
  // char buf[6] = "hello";
  // long rc = syscall(SYS_READ, 0, buf, 5);
  // if (rc <= 0) {
  //   kprintf("read failed\n");
  // } else {
  //   buf[rc] = '\0';
  //   kprintf("read '%s'\n", buf);
  // }

  // test write
  // char buf2[6]; 
  // long rc2 = syscall(SYS_WRITE, 'h', buf2, 5); 
  // if (rc <= 0) {
  //   kprintf("write failed\n"); 
  // } else {
  //   buf2[rc+1] = '\0';
  //   kprintf("wrote '%s'\n", buf2); 
  // }

	// We're done, just hang...
	halt();
}
