#include <stdint.h>

// strtok/strtok_r


// credit: https://stackoverflow.com/questions/3213827/how-to-iterate-over-a-string-in-c 
uint64_t strlen(const char* str) {
  uint64_t len = 0;
  while(*str) { //if we haven't finished iterating over the string, 
    len++; // increase length and 
    str++; // check next character
  }
  return len; 
}

char* kstrcat(char* dest, const char* src, int len) {
  int oglen = strlen(dest); //ignore the null terminator 
  char buf[oglen+len+1]; // +1 for null terminator 

  // move over given from src
  for (int i = 0; i < oglen; i++) {
    buf[i] = *dest; 
    dest++; 
  }

  // add on new stuff
  for (int i = 0; i < len; i++) {
    buf[oglen+i] = *src; 
    src++; 
  }

  // add null terminator
  buf[oglen+len+1] = '\0';

  char* returned = buf; 
  return returned; 
}

char* kstrcpy (char* dest, char* src) {
  int len = strlen(src); 

  // copy over char by char, including null terminator
  for (int i = 0; i < len; i++) {
    *(dest++) = *(src++);  
  }

  return dest; 
}

int kstrcmp(const char *s1, const char *s2) {
  int len1 = strlen(s1); 
  int len2 = strlen(s2); 

  // if the strings are of equal length
  if (len1 == len2) {

    // go through each char and compare
    for (int i = 0; i < len1; i++) {

      // if the chars are the same, check next char
      if (*s1 == *s2) { s1++; s2++; }

      // if not same: 
      else {

        // if the left str char is bigger return 1
        if (*s1 > *s2) { return 1; }

        // otherwise return -1
        else { return -1; }
      }
    }

    // if strings are the same, return 0
    return 0; 
  }

  // if the lengths are not the same
  else {
    
    // if left string is longer, return 1
    if (len1 > len2) { return 1; }

    // otherwise return -1
    else { return -1; }
  }

}

// int katoi(const char *str) { }

// char * kstrsep(char **stringp, const char *delim) { }

// char * kstrpbrk(const char *s, const char *charset) { }

// char * kstrtok(char *restrict str, const char *restrict sep) { }

