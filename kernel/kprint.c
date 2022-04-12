#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "kprint.h"

// typedef void (*term_write_t)(const char*, size_t);
// extern term_write_t term_write;
extern term_putchar(char c); 

// credit: https://stackoverflow.com/questions/3213827/how-to-iterate-over-a-string-in-c 
uint64_t kstrlen(const char* str) {
  uint64_t len = 0;
  while(*str) { //if we haven't finished iterating over the string, 
    len++; // increase length and 
    str++; // check next charactter
  }
  return len; 
}

void kprint_c(char c) {
  term_putchar(c); 
}

void kprint_s(const char* str) {
  uint64_t len = kstrlen(str);
  for (int i = 0; i < len; i++) { term_putchar(str); str++; }
}

void kprint_d(uint64_t value){
  if(value < 10) {
    kprint_c(value + 48);
    return; 
  }

  uint64_t digits[20];  //max number of digits for number 2^64
  uint64_t place = 19; // keeps track of what index of array we're at (remember offset!)
  
  // fill array
  while(value / 10 != 0) {
    digits[place] = value % 10; 
    value = value / 10; 
    if(value < 10) { // if we get down to the last digit, just print it
      kprint_c(value + 48); 
      place++; // fix place to be the previously entered digit
    }
    place --; 
  }

  // print the array: 
  while(place < 20) {
    kprint_c(digits[place] + 48); // 48 is value of 0 in ascii (aka offset is needed)
    place++; 
  }
}

void kprint_x(uint64_t value) {
  if(value < 17) {
    if(value < 10) { kprint_c(value + 48); }
    else { kprint_c(value + 97 - 10); } // add 97 to get to 'a', -10 to find offset within a-f
    return; 
  }

  uint64_t digits[16];  //max number of digits for number 2^64 represented in hex
  uint64_t place = 15; // keeps track of what index of array we're at (remember offset!)
  
  // fill array
  while(value / 16 != 0) {
    digits[place] = value % 16; 
    value = value / 16; 
    if(value < 16) { // if we get down to the last digit, just print it
      if(value < 10) { kprint_c(value + 48); }
      else{ kprint_c(value + 97 - 10); } // add 97 to get to 'a', -10 to find offset within a-f
      place++; // fix place to be the previously entered digit
    }
    place --; 
  }

  // print the array: 
  while (place < 16) {
    if (digits[place] < 10) { kprint_c(digits[place] + 48); } // 48 is value of 0 in ascii (aka offset is needed)
    else { kprint_c(digits[place] + 97 - 10); } // add 97 to get to 'a', -10 to find offset within a-f

    place++; 
  }

}

void kprint_p(void* ptr) {
  kprint_c('0'); 
  kprint_c('x'); 
  kprint_x((uint64_t)ptr); 
  return; 
}

// This is taken directly from the class website after completing the implementation 
//   together as a class 
void kprintf(const char* format, ...) {
  // Start processing variadic arguments
  va_list args;
  va_start(args, format);

  // Loop until we reach the end of the format string
  size_t index = 0;
  while (format[index] != '\0') {
    // Is the current charater a '%'?
    if (format[index] == '%') {
      // Yes, print the argument
      index++;
      switch(format[index]) {
        case '%':
          kprint_c('%');
          break;
        case 'c':
          kprint_c(va_arg(args, int));
          break;
        case 's':
          kprint_s(va_arg(args, char*));
          break;
        case 'd':
          kprint_d(va_arg(args, uint64_t));
          break;
        case 'x':
          kprint_x(va_arg(args, int64_t));
          break;
        case 'p':
          kprint_p(va_arg(args, void*));
          break;
        default:
          kprint_s("<not supported>");
      }
    } else {
      // No, just a normal character. Print it.
      kprint_c(format[index]);
    }
    index++;
  }

  // Finish handling variadic arguments
  va_end(args);
}