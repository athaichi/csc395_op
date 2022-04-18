#pragma once

#include <stdint.h>

uint64_t kstrlen(const char* str); 
char* kstrcat(char* dest, const char* src, int len);