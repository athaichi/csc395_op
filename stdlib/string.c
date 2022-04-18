#include <stdint.h>

// credit: https://stackoverflow.com/questions/3213827/how-to-iterate-over-a-string-in-c 
uint64_t kstrlen(const char* str) {
  uint64_t len = 0;
  while(*str) { //if we haven't finished iterating over the string, 
    len++; // increase length and 
    str++; // check next character
  }
  return len; 
}

char* kstrcat(char* dest, const char* src, int len) {
  int oglen = kstrlen(dest); 
  char buf[oglen+len+1]; // +1 for null terminator 

  // move over given from src
  for (int i = 0; i < oglen; i++) {
    buf[i] = *dest; 
    dest++; 
  }

  // add on new stuff
  for (int i = 0; i < len; i++) {
    buf[oglen+i] = *src; 
    src++; 
  }

  // add null terminator
  buf[oglen+len+1] = '\0';

  char* returned = buf; 
  return returned; 
}

