#include "process.h"
#include "../kernel/executables.h"

void exec(char* modulename) {
    exec_setup(modulename);   
}

void exit() {
    exec_setup("init"); 
}

// long wait(int secs);
//long rc fork();