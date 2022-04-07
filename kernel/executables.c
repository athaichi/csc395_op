#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "kprint.h"
#include "memory.h"

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
    elf_word sh_name;       //section name
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

// struct for an elf64 program header table 
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

void find_modules(struct stivale2_struct* hdr) {
  struct stivale2_struct_tag_modules* modules = find_tag(hdr, STIVALE2_STRUCT_TAG_MODULES_ID);

  for(uint64_t current = 0; current < modules->module_count; current++) {
    struct stivale2_module cur = modules->modules[current]; 
    
    //print all the modules
    //print name
    for (int i = 0; i<128; i++) {
      //char n = cur.string[i]; 
      //kprint_c(n); 
      //if (n == '\0') { break; }
    }
    kprint_s("    0x");
    kprint_x(cur.begin); 
    kprint_s(" - 0x"); 
    kprint_x(cur.end); 

    // format for next entry 
    kprint_c('\n');
  }
}