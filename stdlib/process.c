#include "process.h"
#include "../kernel/executables.h"

/**
 * Stdlib wrapper for syscall exec
 *
 * \param modulename which module to execute
 */
void exec(char* modulename) {
    exec_setup(modulename);   
}

/**
 * Stdlib wrapper for syscall exit
 */
void exit() {
    exec_setup("init"); 
}

// long wait(int secs);
//long rc fork();