  #include <stdint.h>
  #include "kprint.h"

  void kprint_tests() {

  // test kprint_c
  char test = 'h';  
  kprint_c(test);
  test = 'H'; 
  kprint_c(test);
  kprint_c(' ');
  kprint_c('}');
  kprint_c('Q');
  kprint_c('q'); 
  kprint_c('\n');

  // test kprint_d
  kprint_d(2);
  kprint_c('\n'); 
  kprint_d(0); 
  kprint_c('\n'); 
  kprint_d(1000); 
  kprint_c('\n'); 
  kprint_d(1342); 
  kprint_c('\n'); 
  kprint_d(5379); 

  //test kprint_s
  kprint_s("hello\n");
  kprint_s("hi! nice to meet you????\n"); 
  kprint_s("4384>?<P{_)(*&^$#@!\t check tabs"); 

  // test strlen
  kprint_d(kstrlen("hello")); // should be 5
  kprint_c(' ');  
  kprint_d(kstrlen("hi")); // should be 2
  kprint_c(' '); 
  kprint_d(kstrlen("hey this is joe")); // should be 15

  // test kprint_x
  kprint_x(0); // should be 0
  kprint_c(' '); 
  kprint_x(10); // should be a
  kprint_c(' ');
  kprint_x(48362); // should be bcea
  kprint_c(' ');
  kprint_x(4738295); // should be 484cf7

  // test kprint_p
  kprint_p(kprint_tests); 
}

void all_tests() {
    kprint_tests(); 
}