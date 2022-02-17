#pragma once

uint64_t kstrlen(const char* str); 
void kprint_c(char c); 
void kprint_s(const char* str); 
void kprint_d(uint64_t value); 
void kprint_x(uint64_t value); 
void kprint_p(void* ptr); 
void kprintf(const char* format, ...); 