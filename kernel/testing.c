#include <stdint.h>

#include <string.h>

#include "kprint.h"

// void kprint_tests() {

//   // test kprint_c
//   char test = 'h';  
//   kprint_c(test);
//   test = 'H'; 
//   kprint_c(test);
//   kprint_c(' ');
//   kprint_c('}');
//   kprint_c('Q');
//   kprint_c('q'); 
//   kprint_c('\n');

//   // test kprint_d
//   kprint_d(2);
//   kprint_c('\n'); 
//   kprint_d(0); 
//   kprint_c('\n'); 
//   kprint_d(1000); 
//   kprint_c('\n'); 
//   kprint_d(1342); 
//   kprint_c('\n'); 
//   kprint_d(5379); 

//   //test kprint_s
//   kprint_s("hello\n");
//   kprint_s("hi! nice to meet you????\n"); 
//   kprint_s("4384>?<P{_)(*&^$#@!\t check tabs"); 

//   // test strlen
//   kprint_d(kstrlen("hello")); // should be 5
//   kprint_c(' ');  
//   kprint_d(kstrlen("hi")); // should be 2
//   kprint_c(' '); 
//   kprint_d(kstrlen("hey this is joe")); // should be 15

//   // test kprint_x
//   kprint_x(0); // should be 0
//   kprint_c(' '); 
//   kprint_x(10); // should be a
//   kprint_c(' ');
//   kprint_x(48362); // should be bcea
//   kprint_c(' ');
//   kprint_x(4738295); // should be 484cf7

//   // test kprint_p
//   kprint_p(kprint_tests); 
// }

//   //translate(_start, hdr);
//   //translate(stack, hdr);  
//   //translate(usable_memory, hdr);
//   //translate(NULL, hdr); 

//   // kprintf("interrupt should be above this\n"); 

//   // mem_init(hdr);
//   // kprintf("init finished\n");

//   // // test vm_map()
//   // // Enable write protection
  // uint64_t cr0 = read_cr0();
  // cr0 |= 0x10000;
  // write_cr0(cr0);
  // //    with unmapped addresses
  // uintptr_t root = read_cr3() & 0xFFFFFFFFFFFFF000;
  // int* p = (int*)0x50004000;
  // bool result = vm_map(root, (uintptr_t)p, false, true, false);
  // if (result) {
  //   *p = 123;
  //   kprintf("Stored %d at %p, number of free pages is now %d\n", *p, p, free_page_counter);
  // } else {
  //  kprintf("vm_map failed with an error, freepage number unchanged %d\n", free_page_counter);
  // }

  // p = (int*)0x54739500; 
  // result = vm_map(root, (uintptr_t)p, false, true, false);
  // if (result) {
  //   *p = 123;
  //   kprintf("Stored %d at %p, number of free pages is now %d\n", *p, p, free_page_counter);
  // } else {
  //  kprintf("vm_map failed with an error, freepage number unchanged %d\n", free_page_counter);
  // }

  // //    with mapped address
  // result = vm_map(root, (uintptr_t)p, false, true, false);
  // if (result) {
  //   *p = 123;
  //   kprintf("Stored %d at %p, number of free pages is now %d\n", *p, p, free_page_counter);
  // } else {
  //  kprintf("vm_map failed with an error, freepage number unchanged %d\n", free_page_counter);
  // }

  // // test unmap
  // //    with mapped entry
  // result = vm_unmap(root, (uintptr_t)p); // unmapping address 0x54739500
  // if (result) {
  //   kprintf("Removed %d at %p, number of free pages is now %d\n", *p, p, free_page_counter);
  // } else {
  //  kprintf("vm_unmap failed with an error\n");
  // }

  // //    with unmapped entry
  // p = (int*)0x500430020; 
  // result = vm_unmap(root, (uintptr_t)p); 
  // if (result) {
  //   kprintf("Removed %d at %p, number of free pages is now %d\n", *p, p, free_page_counter);
  // } else {
  //  kprintf("vm_unmap failed with an error, free page num unchanged %d\n", free_page_counter);
  // }

  // // test protect - but badly
  // //    with mapped value
  // p = (int*)0x50004000; // from first vm_map test
  // result = vm_protect(root, (uintptr_t)p, false, true, true); 
  // if (result) {
  //   kprintf("Changed protections at %p, \nnumber of free pages should be the same %d\n", p, free_page_counter);
  // } else {
  //   kprintf("vm_protect failed with an error, freepage num unchanged %d\n", free_page_counter); 
  // }

  // //    with unmapped value
  // p = (int*)0x500430020; // same unmapped value used with vm_unmap test
  // result = vm_protect(root, (uintptr_t)p, false, true, true); 
  // if (result) {
  //   kprintf("Changed protections at %p, \nnumber of free pages should be the same %d\n", p, free_page_counter);
  // } else {
  //   kprintf("vm_protect failed with an error, freepage num unchanged %d\n", free_page_counter); 
  // }

  // // test write
  // // syscall nr, file descriptor nr, buffer, buf length
  // long rc2 = syscall(SYS_WRITE, 1, "olleh", 5); 
  // if (rc2 < 0) {
  //   kprintf("write failed\n"); 
  // } else {
  //   kprintf("write success!\n"); 
  // }

    // test usable_memory
  //usable_memory(hdr); 

  // test idt
  //  int* p = (int*)0x1;
  //  *p = 123; 
  //__asm__("int $2");


  // test paging
  //translate(read_cr3(), _start); 

  // test kgetc()
  // while(1) {
  //   kprintf("%c", kgetc()); 
  // }

  // // // test write
  // char buf2[6] = "olleh"; 
  // long rc2 = syscall(SYS_WRITE, 1, buf2, 6); 
  // if (rc2 < 0) {
  //   kprintf("write failed\n"); 
  // } else {
  //   buf2[rc2] = '\0';
  //   kprintf("wrote '%s'\n", buf2); 
  // }

  // // test read
  // char buf[6];
  // long rc = syscall(SYS_READ, 0, buf, 5);
  // if (rc < 0) {
  //   kprintf("read failed\n");
  // } else {
  //   buf[rc] = '\0';
  //   kprintf("read '%s', and rc: %d\n", buf, rc);
  // }


  // test strcat and memcpy
  // char* test = "taco";
  // char* new = NULL;  
  // char copy[5]; 
  // char c = 's'; 
  // //new = kstrcat(test, &c, 1); 
  // //kprintf("%s", new); 
  // k_memcpy(copy, test, 5); 
  // kprintf("%s", copy); 

  //test exec_setup()
  //exec_setup(hdr);

  // char* og = "hello!"; 
  // char dup[7]; 
  // kstrcpy(dup, og); 
  // kprintf("copied string is: %s\n", dup); 
  // int ret = kstrcmp(og, dup); 
  // kprintf("strings should be the same: %d\n", ret);  
  // char * add = "hello!"; 
  // char * new = kstrcat(dup, add, 6); 
  // kprintf("new str is: %s\n", new); 
  // ret = kstrcmp(og, new); 
  // kprintf("strs should not be the same: %d\n", ret); 

void all_tests() {
   // kprint_tests(); 
}