#include <stdint.h>

#include <string.h>

#include "systemcalls.h"
#include "kprint.h"
#include "page.h"



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

// system call read() function 
// fix this to handle backspace
int kread(uint64_t buf, uint64_t numchars) {
  // create a new buffer
  char* buff = (char*)buf;   

  // fill the new buffer using kgetc
  for (int i = 0; i < numchars; i++) {
    char ret = kgetc(); 
    buff[i] = ret;
    kprintf("%c", ret); 
  }

  // set up to return 
  return numchars; 
}

int kwrite(uint64_t buf, uint64_t len) {
  // turn buffer into a string
  char* wbuffer = (char*)buf; 

  // kprint the string 
  for (uint64_t i = 0; i < len; i++) {
    kprint_c(*wbuffer); 
    wbuffer++; 
  }

  return len; 
}

void char_write(char key) {
  if (key == '\0') {
    return; 
  }

  kprintf("%c", key); 
  // add it to the buffer
  buffer[writing_index] = key; 
  writing_index++; 
  buffer_length++; 

  // if we reach the end of the index, loop back to the beginning
  if (writing_index >= 100) {
    writing_index = 0; 
  }
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

  // get character off of buffer
  char returned = buffer[reading_index]; 

  // move the pointer to the next character to read and decrease buffer length
  reading_index++; 
  buffer_length--; 

  // check if we need to loop back around to the beginning of the array
  if (reading_index >= 100) { 
    reading_index = 0; 
  } 

  // // check to see if it's actually useful
  // if (returned == '\0') { kgetc(); }

  //kprintf("interrput has happened and we're back in kgetc()...")
  //kprintf("about to hit the return\n");
  
  return returned; 
}

// starting address for mapping space
uintptr_t MAPSTART = 0x90000000000;

//mmap from syscall side
uintptr_t map(void *addr, size_t len, int prot, int flags, int fd, int offset) { 
  uintptr_t startmap = 0; 

  // get protection flags
  bool writable = false, executable = false, usable = false; 
  if((prot & PROT_EXEC) > 0) { executable = true; }
  if((prot & PROT_WRITE) > 0) { writable = true; }
  if((prot & PROT_NONE) > 0) {  }

  // get map flags
  bool shared = false, private = false, anonymous = false, fixed = false; 
  if((flags & MAP_ANONYMOUS) > 0) { anonymous = true; }
  if((flags & MAP_FIXED) > 0) { fixed = true; }
  if((flags & MAP_PRIVATE) > 0) { private = true; }
  if((flags & MAP_SHARED) > 0) { shared = true; }

  // decide on a start address that is page aligned
  if (fixed) {
    startmap = (uintptr_t)addr; 
  } else if (addr != NULL) {
    int off = (uintptr_t)addr % PAGESIZE; 
    startmap = (uintptr_t)addr - off; 
  } else {
    startmap = MAPSTART; 
  }

  // loop thorough and vm_map appropriate amount of space
  for (int i = 0; i < len; i+=PAGESIZE) {
    bool ret = vm_map(read_cr3() & 0xFFFFFFFFFFFFF000, startmap, usable, writable, executable);
    if(!ret) {
      kprintf("MMAP has failed\n"); 

      // reuse half mapped space
      MAPSTART = MAPSTART - (PAGESIZE * i); 
      return -1;  
    }

    // update MAPSTART position
    MAPSTART += PAGESIZE; 
  }

  return startmap; 
}

// SYSCALL STUFF:
int64_t syscall_handler(uint64_t nr, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
  //kprintf("syscall %d: %d, %d, %d, %d, %d, %d\n", nr, arg0, arg1, arg2, arg3, arg4, arg5);
  
  // if we are reading, call the read function
  if (nr == SYS_READ) {int rlen = kread(arg1, arg2); return rlen; }

  // if we are writing, call the write function
  if (nr == SYS_WRITE) {int wlen = kwrite(arg1, arg2); return wlen; }

  // if we are mmaping, call map
  if (nr == SYS_MAP) {uint64_t ret = (uint64_t)map((void*)arg0, arg1, arg2, arg3, arg4, arg5); return ret; }

  // file descriptor is not allowed
  return -1;
}

extern int64_t syscall(uint64_t nr, ...);
extern void syscall_entry();