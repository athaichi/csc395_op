#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <io.h>
#include <process.h>
#include <string.h>

extern int syscall(uint64_t nr, ...);
//extern struct module_t; 

typedef struct modlist {
  char * name; 
  struct modlist * next;
} __attribute__((packed)) modlist_t;

bool askinput(char* modname, modlist_t* modlist, int modnum) {
  char* curchar = NULL; 
  int counter = 0; 

  // read in chars until we reach a newline
  while(counter < 20) {
    read(STDIN, curchar, 1); 

    // if user input is done
    if (*curchar == '\n') {

      // see if the command matches an available module name
      for (int i = 0; i < modnum; i++) {
        int ret = strcmp(modname, modlist->name); 
        
        // match! return true so we can exec back in _start
        if (ret == 0) {
          return true; 
        }

        modlist = modlist->next; 
      }

      // no dice, return false
      return false; 
    }

    // if input is not done, put it into modname
    strcat(modname, curchar, 1); 
  }

  // if you're here, input is invalid 
  return false; 

}

void _start() {

  // Issue a write system call
  write(STDOUT, "Init start!\n$");
  
  // get a list of all the modules
  modlist_t* modlist = (modlist_t*)getmodules(); 
  int modnum = getmodnums(); 

  char modname[20]; 

  // read in user input: 
  bool valid = askinput(modname, modlist, modnum); 

  // if command is not valid, ask for a new input
  while(!valid) {
    write(STDOUT, "   Invalid command.\n$");  
    askinput(modname, modlist, modnum); 
  }
  
  // if inputs match module names, exec
  exec(modname); 
  
  

  // Loop forever
  for(;;){}
}
