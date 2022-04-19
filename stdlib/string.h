#pragma once

#include <stdint.h>

uint64_t strlen(const char* str); 
char* kstrcat(char* dest, const char* src, int len);
char* kstrcpy (char* dest, char* src); 
int kstrcmp(const char *s1, const char *s2);
// int katoi(const char *str);
// char * kstrsep(char **stringp, const char *delim);
// char * kstrpbrk(const char *s, const char *charset);
// char * kstrtok(char *restrict str, const char *restrict sep);
