#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "kprint.h"
#include "memory.h"
#include "page.h"

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

// implementation from https://www.geeksforgeeks.org/write-memcpy/ 
void k_memcpy (void* src, void* dest, uint64_t size) {
    char* csrc = (char*)src; 
    char* cdest = (char*)dest; 

    for (int i = 0; i < size; i++) {
        cdest[i] = csrc[i]; 
    }
}


void exec_setup(struct stivale2_struct* hdr) {
    // find a module - right now hardcoded to the first module
    struct stivale2_struct_tag_modules* moduleslist = find_tag(hdr, STIVALE2_STRUCT_TAG_MODULES_ID);
    struct stivale2_module ourmod = moduleslist->modules[0]; 

    kprintf("got modules...\n"); 

    // cast it to an elf header 
    elf_hdr_t* header = (elf_hdr_t*)(ourmod.begin); 

    // locate the program header table
    elf_phdr_t* ph_entry = (elf_phdr_t*)(header + header->e_phoff); 
    kprintf("got to the header...\n"); 

    // get physical address
    uintptr_t cr3 = read_cr3() & 0xFFFFFFFFFFFFF000;

    // loop over the entries 
    for (uint16_t i = 0; i < header->e_phnum; i++) {
        kprintf("in loop, on round %d. type = %d\n", i, ph_entry->p_type); 

        // if entry has type LOAD and size > 0
        if ((ph_entry->p_type == LOAD) && (ph_entry->p_filesz > 0)) {
            kprintf("got into if\n"); 

            // vm_map for the entry - init set as non-executable
            bool ret = vm_map(cr3, ph_entry->p_vaddr, true, true, false); 
            if (ret) {kprintf("vm mapped for section %d out of %d...\n", i, header->e_phnum); }

            // memcpy data into the virtual address
            k_memcpy(ph_entry + ph_entry->p_offset, (uintptr_t*)(ph_entry->p_vaddr), ph_entry->p_memsz); 

            // get flags, writable = 0x2, executable = 0x1
            bool writable = false, executable = false; 
            if((ph_entry->p_flags & X) > 0) { executable = true; }
            if((ph_entry->p_flags & W) > 0) { writable = true; }

            // update permissions -- always set usable to be true
            vm_protect(read_cr3(), ph_entry->p_vaddr, true, writable, executable); 
        }

        // move to next program header entry
        ph_entry += (uint64_t)(sizeof(ph_entry)); 
    }
    
    // cast entry point to a function pointer and run!
    typedef void (*entry_fn_ptr_t)(); 

    entry_fn_ptr_t entry = (entry_fn_ptr_t)header->e_entry; 
    kprintf("got to end\n"); 
    //entry(); -- this is a page fault currently
}

// void find_modules(struct stivale2_struct* hdr) {
//   struct stivale2_struct_tag_modules* modules = find_tag(hdr, STIVALE2_STRUCT_TAG_MODULES_ID);

//   for(uint64_t current = 0; current < modules->module_count; current++) {
//     struct stivale2_module cur = modules->modules[current]; 
    
//     //print all the modules
//     //print name
//     for (int i = 0; i<128; i++) {
//       //char n = cur.string[i]; 
//       //kprint_c(n); 
//       //if (n == '\0') { break; }
//     }
//     kprint_s("    0x");
//     kprint_x(cur.begin); 
//     kprint_s(" - 0x"); 
//     kprint_x(cur.end); 

//     // format for next entry 
//     kprint_c('\n');
//   }
// }