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

// create an array of keys following scan code from 
//   https://wiki.osdev.org/PS2_Keyboard#Scan_Code_Set_1
//   Everett Hayes sent me the keyboard layout code
char keys[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', /* <-- Tab */
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',     
    0, /* <-- control key */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',  0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
    '+',
    0,
    0,
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

// circular buffer and read/write pointers
char buffer[100]; 
volatile int8_t buffer_length = 0; // number of characters currently in the buffer
int8_t reading_index = 0; 
int8_t writing_index = 0; 

void backspace() {
  buffer_length--; 
  reading_index--;

  // check if we need to loop back around to the beginning of the array
  if (reading_index < 0) { 
    reading_index = 99; 
  }
}

// take in a code and print corresponding key
char getkey(uint8_t code) {

  // Handle fancy characters
  //========================
  // esc
  if (code == 0x01) { return '\0'; }
  
  // backspace
  if (code == 0x0E) { 
    backspace();
    return '\0'; }

  // left control
  if (code == 0x1D) { return '\0'; }

  // left/right shift
  if ((code == 0x2A) || (code == 0x36)) { return '\0'; }

  // left alt
  if (code == 0x38) { return '\0'; }

  // caps loc
  if (code == 0x3A) { return '\0'; }
  
  //keyup codes
  if (code >= 0x81) { return '\0'; }

  // Handle everything else
  // ======================
  return keys[code]; 
}

char read() {
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

void write(char key) {
  // add it to the buffer
  buffer[writing_index] = key; 
  writing_index++; 
  buffer_length++; 

  // if we reach the end of the index, loop back to the beginning
  if (writing_index >= 100) {
    writing_index = 0; 
  }

}



// interrupt for key presses
__attribute__((interrupt))
void keyboard_interrupt(interrupt_context_t* ctx) {
  // get the keycode
  uint8_t code = inb(0x60);

  // make it the actual character
  char key = getkey(code);  

  // FOR TESTING
  // print character
  //kprint_c(key); 

  // write character to buffer
  write(key); 

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

  char returned = read(); 

  //kprintf("interrput has happened and we're back in kgetc()...")
  //kprintf("about to hit the return\n");
  
  return returned; 
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
  if (nr == SYS_READ) {read(); return arg2; }

  // if we are writing, call the write function
  if (nr == SYS_WRITE) {write(arg0); return arg2; }

  return -1;
}



extern int64_t syscall(uint64_t nr, ...);
extern void syscall_entry();
 

// -------------------------------------------------

void _start(struct stivale2_struct* hdr) {
  // We've booted! Let's start processing tags passed to use from the bootloader
  term_setup(hdr);
  idt_setup(); 

  // Keyboard stuff
  pic_init(); 
  pic_unmask_irq(1); 

  // set up customized handler functions: 
  // allow us to make system calls!
  idt_set_handler(0x80, syscall_entry, IDT_TYPE_TRAP);
  // allow us to handle keyboard interrupts
  idt_set_handler(IRQ1_INTERRUPT, keyboard_interrupt, IDT_TYPE_TRAP);

  // Print a greeting
  term_write("Hello Kernel!\n", 14);

  //all_tests(); 

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
  char buf[6] = "hello";
  long rc = syscall(SYS_READ, 0, buf, 5);
  if (rc < 0) {
    kprintf("read failed\n");
  } else {
    buf[rc] = '\0';
    kprintf("read '%s'\n", buf);
  }

  // test write
  char buf2[6]; 
  long rc2 = syscall(SYS_WRITE, 'h', buf2, 1); 
  if (rc2 < 0) {
    kprintf("write failed\n"); 
  } else {
    rc = syscall(SYS_READ, 0, buf2, 5); 
    buf2[rc] = '\0';
    kprintf("wrote '%s'\n", buf2); 
  }

	// We're done, just hang...
	halt();
}
