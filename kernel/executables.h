#pragma once

#include "stivale2.h"

// create a linked list of module names  
typedef struct modname {
  char * name; 
  struct modname * next;
} __attribute__((packed)) modname_t;

void exec_setup(char* modulename); 
void* modnames(); 
int modnums();
