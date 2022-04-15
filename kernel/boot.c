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
#include "systemcalls.h"
#include "executables.h"
//#include "../usermode/gdt.h"



// Reserve space for the stack
static uint8_t stack[8192];

// for testing interrupts
static struct stivale2_tag unmap_null_hdr_tag = {
  .identifier = STIVALE2_HEADER_TAG_UNMAP_NULL_ID,
  .next = 0
};

// ==============================================================
// | Terminal Functions |
// ======================

// Request a terminal from the bootloader
static struct stivale2_header_tag_terminal terminal_hdr_tag = {
	.tag = {
    .identifier = STIVALE2_HEADER_TAG_TERMINAL_ID,
    .next = (uintptr_t)&unmap_null_hdr_tag
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

#define VGA_BUFFER 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define VGA_COLOR_BLACK 0
#define VGA_COLOR_BLUE 1
#define VGA_COLOR_GREEN 2
#define VGA_COLOR_CYAN 3
#define VGA_COLOR_RED 4
#define VGA_COLOR_MAGENTA 5
#define VGA_COLOR_BROWN 6
#define VGA_COLOR_LIGHT_GREY 7
#define VGA_COLOR_DARK_GREY 8
#define VGA_COLOR_LIGHT_BLUE 9
#define VGA_COLOR_LIGHT_GREEN 10
#define VGA_COLOR_LIGHT_CYAN 11
#define VGA_COLOR_LIGHT_RED 12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN 14
#define VGA_COLOR_WHITE 15

// Struct representing a single character entry in the VGA buffer
typedef struct vga_entry {
  uint8_t c;
  uint8_t fg : 4;
  uint8_t bg : 4;
} __attribute__((packed)) vga_entry_t;

// A pointer to the VGA buffer
vga_entry_t* term;

// The current cursor position in the terminal
size_t term_col = 0;
size_t term_row = 0;

// Turn on the VGA cursor
void term_enable_cursor() {
  // Set starting scaline to 13 (three up from bottom)
  outb(0x3D4, 0x0A);
  outb(0x3D5, (inb(0x3D5) & 0xC0) | 13);
 
  // Set ending scanline to 15 (bottom)
  outb(0x3D4, 0x0B);
  outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}

// Update the VGA cursor
void term_update_cursor() {
  uint16_t pos = term_row * VGA_WIDTH + term_col;
 
  outb(0x3D4, 0x0F);
  outb(0x3D5, (uint8_t) (pos & 0xFF));
  outb(0x3D4, 0x0E);
  outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

// Clear the terminal
void term_clear() {
  // Clear the terminal
  for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
    term[i].c = ' ';
    term[i].bg = VGA_COLOR_BLACK;
    term[i].fg = VGA_COLOR_WHITE;
  }

  term_col = 0;
  term_row = 0;

  term_update_cursor();
}

// Write one character to the terminal
void term_putchar(char c) {
  // Handle characters that do not consume extra space (no scrolling necessary)
  if (c == '\r') {
    term_col = 0;
    term_update_cursor();
    return;

  } else if (c == '\b') {
    if (term_col > 0) {
      term_col--;
      term[term_row * VGA_WIDTH + term_col].c = ' ';
    }
    term_update_cursor();
    return;
  }

  // Handle newline
  if (c == '\n') {
    term_col = 0;
    term_row++;
  }

  // Wrap if needed
  if (term_col == VGA_WIDTH) {
    term_col = 0;
    term_row++;
  }

  // Scroll if needed
  if (term_row == VGA_HEIGHT) {
    // Shift characters up a row
    k_memcpy(term, &term[VGA_WIDTH], sizeof(vga_entry_t) * VGA_WIDTH * (VGA_HEIGHT - 1));
    term_row--;
    
    // Clear the last row
    for (size_t i=0; i<VGA_WIDTH; i++) {
      size_t index = i + term_row * VGA_WIDTH;
      term[index].c = ' ';
      term[index].fg = VGA_COLOR_WHITE;
      term[index].bg = VGA_COLOR_BLACK;
    }
  }

  // Write the character, unless it's a newline
  if (c != '\n') {
    size_t index = term_col + term_row * VGA_WIDTH;
    term[index].c = c;
    term[index].fg = VGA_COLOR_WHITE;
    term[index].bg = VGA_COLOR_BLACK;
    term_col++;
  }

  term_update_cursor();
}

// Initialize the terminal
void term_init() {
  // Get a usable pointer to the VGA text mode buffer
  term = ptov(VGA_BUFFER);

  term_enable_cursor();
  term_clear();
}

// ==========================================================
// | END TERMINAL |
// ================

// gets set in init_init
uintptr_t hhdm_base = 0; 

void init_init(struct stivale2_struct* hdr) {
  // set base for page.c and boot. 
  hhdm_base = get_hhdm(hdr); 

}

// void term_setup(struct stivale2_struct* hdr) {
//   // Look for a terminal tag
//   struct stivale2_struct_tag_terminal* tag = find_tag(hdr, STIVALE2_STRUCT_TAG_TERMINAL_ID);

//   // Make sure we find a terminal tag
//   if (tag == NULL) halt();

//   // Save the term_write function pointer
// 	term_write = (term_write_t)tag->term_write;
// }

// --------------------------------------------------
// =======================================================
// =======================================================

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
  char_write(key); 

  outb(PIC1_COMMAND, PIC_EOI); 
}

// ==============================================
// ==============================================
// // ===============================================
// // ===============================================


// global! keep track of number of free pages - just for testing purposes
extern int free_page_counter;

uint64_t read_cr0() {
  uintptr_t value;
  __asm__("mov %%cr0, %0" : "=r" (value));
  return value;
}

void write_cr0(uint64_t value) {
  __asm__("mov %0, %%cr0" : : "r" (value));
}

// -------------------------------------------------

void _start(struct stivale2_struct* hdr) {
  // -------------------------------------------
  // | SETUP |
  // ---------
  // We've booted! Let's start processing tags passed to use from the bootloader
  init_init(hdr); 
  term_init();
  //term_setup(hdr);
  gdt_setup(); 
  idt_setup(); 
  mem_init(hdr); 

  // Keyboard stuff
  pic_init(); 
  pic_unmask_irq(1); 

  // set up customized handler functions: 
  // allow us to make system calls!
  idt_set_handler(0x80, syscall_entry, IDT_TYPE_TRAP);
  // allow us to handle keyboard interrupts
  idt_set_handler(IRQ1_INTERRUPT, keyboard_interrupt, IDT_TYPE_TRAP);

  // Print a greeting
  kprintf("Hello Kernel!\n"); //term_write("Hello Kernel!\n", 14);

  //all_tests(); 

  // get modules
  // kprint_s("Modules: \n"); 
  // find_modules(hdr); 

	// We're done, just hang...
	halt();
}
