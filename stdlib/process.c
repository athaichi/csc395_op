#include "process.h"
#include "../kernel/systemcalls.h"

//extern void exec_setup(char* modulename); 

void exec(char* modulename) {
    //exec_setup(modulename); 
    syscall(SYS_EXEC, modulename);  
}

void exit() {
    exec("init"); 
}


// long wait(int secs);
//long rc fork();