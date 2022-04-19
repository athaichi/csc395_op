#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <memory.h>
#include <string.h>

#include "kprint.h"
#include "page.h"
#include "gdt.h"
#include "mem.h"
#include "executables.h"

#define PAGESIZE 0x1000
#define MAX_MODNAME_LENGTH 30


// ---------------------------------------------------------
// | Definitions & Structs |  // NOTE: These are taken directly from ELF64 specifications
// -------------------------

// Definitions for elf specifications
#define elf_addr uint64_t
#define elf_offset uint64_t
#define elf_half uint16_t
#define elf_word uint32_t
#define elf_sword int32_t
#define elf_xword uint64_t
#define elf_sxword int64_t

// Definitions for program headers
#define LOAD 1      // loadable section
#define W 0x2       // write premission 
#define X 0x1       // execute permission
#define R 0x4       // read permission

// Jump to usermode 
extern void usermode_entry(uint64_t data_sel, uintptr_t stack_ptr, uint64_t code_sel, uintptr_t instruction_ptr, ...);

// Struct for an elf64 header 
typedef struct elf_hdr {
    char e_ident[16];       // elf identification
    elf_half e_type;        // object file type
    elf_half e_machine;     // machine type
    elf_word e_version;     // object file version
    elf_addr e_entry;       // entry point 
    elf_offset e_phoff;     // program header offset
    elf_offset e_shoff;     // section header offset
    elf_word e_flags;       // process-specific flags
    elf_half e_ehsize;      // elf header size
    elf_half e_phentsize;   // size of program header entry
    elf_half e_phnum;       // number of program headers
    elf_half e_shentsize;   // size of section header entries
    elf_half e_shnum;       // number of section header entries
    elf_half e_shstrndx;    // section name string table
} __attribute__((packed)) elf_hdr_t;

// struct for an elf64 section header
typedef struct elf_shdr {
    elf_word sh_name;       // section name
    elf_word sh_type;       // section type
    elf_xword sh_flags;     // section attributes
    elf_addr sh_addr;       // virtual address in memory
    elf_offset sh_offset;   // offset in file
    elf_xword sh_size;      // size of section
    elf_word shh_link;      // link to othher section
    elf_word sh_info;       // misc information
    elf_xword sh_addralign; // address alignment boundary
    elf_xword sh_enttsize;  // size of entries, if section has a table
} __attribute__((packed)) elf_shdr_t; 

// struct for an elf64 program header table entry
typedef struct elf_phdr {
    elf_word p_type;        // type of segment
    elf_word p_flags;       // segment attributes
    elf_offset p_offset;    // offset in file
    elf_addr p_vaddr;       // virtual address in memory
    elf_addr p_paddr;       // reserved
    elf_xword p_filesz;     // size of segment in file
    elf_xword p_memsz;      // size of segment in memory
    elf_xword p_align;      // alignment of segment
} __attribute__((packed)) elf_phdr_t; 

// ---------------------------------------------------------------

// hdr from boot.c _start
extern struct stivale2_struct* hdr;

// set up pointer to the front of the list
modname_t * modnamelist = NULL; 

void * modnames () {
    // find the list of modules
    struct stivale2_struct_tag_modules* moduleslist = find_tag(hdr, STIVALE2_STRUCT_TAG_MODULES_ID);
    int modnum = moduleslist->module_count;

    // set up a curmod
    struct stivale2_module curmod; 

    // go through and add a new list entry per mod
    for (int i = 0; i < modnum; i++) {
        curmod = moduleslist->modules[i]; 
        modname_t * new = NULL; 
        new->name = curmod.string; 

        // update head of linked list
        new->next = modnamelist; 
        modnamelist = new;  
    } 

    return (void*)modnamelist; 
}

int modnum() {
    // get list of modules
    struct stivale2_struct_tag_modules* moduleslist = find_tag(hdr, STIVALE2_STRUCT_TAG_MODULES_ID);
    int modnum = moduleslist->module_count;
    return modnum; 
}

void exec_setup(char* modulename) {
    // find correct module
    struct stivale2_struct_tag_modules* moduleslist = find_tag(hdr, STIVALE2_STRUCT_TAG_MODULES_ID);
    struct stivale2_module ourmod;
    bool found = false; // set this to one when we find the module

    for (int i = 0; i < moduleslist->module_count; i++) {
        ourmod = moduleslist->modules[i]; 
        int ret = strcmp(modulename, ourmod.string); 
        
        // we've found the right module
        if (ret == 0) {
            found = true; 
            break; 
        } 
    } 

    // make sure the module is found
    if (!found) {
        kprintf("An error occured when trying to find the module. \n"); 
        return; 
    }

    // cast it to an elf header 
    elf_hdr_t* header = (elf_hdr_t*)(ourmod.begin); 

    // locate the program header table
    elf_phdr_t* ph_entry = (elf_phdr_t*)((uintptr_t)header + header->e_phoff); 

    // get physical address
    uintptr_t cr3 = read_cr3() & 0xFFFFFFFFFFFFF000;

    // loop over the entries 
    for (uint16_t i = 0; i < header->e_phnum; i++) {

        // if entry has type LOAD and size > 0
        if ((ph_entry->p_type == LOAD) && (ph_entry->p_filesz > 0)) {
            //kprintf("got into if\n"); 

            // vm_map for the entry - init set as non-executable
            bool ret = vm_map(cr3, ph_entry->p_vaddr, true, true, false); 
            
            // memcpy data into the virtual address
            // if file size is 0 use filesz otherwise use memsz
            if (ph_entry->p_filesz == 0) {
               memcpy((uintptr_t*)(ph_entry->p_vaddr), (uintptr_t*)((uintptr_t)header + ph_entry->p_offset),  ph_entry->p_filesz); 
            } else {
                memcpy((uintptr_t*)(ph_entry->p_vaddr), (uintptr_t*)((uintptr_t)header + ph_entry->p_offset),  ph_entry->p_memsz); 
            }
            
            // get flags, writable = 0x2, executable = 0x1
            bool writable = false, executable = false; 
            if((ph_entry->p_flags & X) > 0) { executable = true; }
            if((ph_entry->p_flags & W) > 0) { writable = true; }

            // update permissions -- always set usable to be true
            vm_protect(read_cr3(), ph_entry->p_vaddr, true, writable, executable); 
        }

        // move to next program header entry
        ph_entry++; 
    }
    

    // Pick an arbitrary location and size for the user-mode stack
    uintptr_t user_stack = 0x70000000000;
    size_t user_stack_size = 8 * PAGESIZE;

    // Map the user-mode-stack
    for(uintptr_t p = user_stack; p < user_stack + user_stack_size; p += 0x1000) {
        // Map a page that is user-accessible, writable, but not executable
        vm_map(read_cr3() & 0xFFFFFFFFFFFFF000, p, true, true, false);
    }

    // And now jump to the entry point
    usermode_entry(USER_DATA_SELECTOR | 0x3,            // User data selector with priv=3
                    user_stack + user_stack_size - 8,   // Stack starts at the high address minus 8 bytes
                    USER_CODE_SELECTOR | 0x3,           // User code selector with priv=3
                    header->e_entry);                   // Jump to the entry point specified in the ELF file

}