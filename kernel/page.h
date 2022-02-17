#pragma once

uintptr_t read_cr3();
void translate(uintptr_t page_table, void* address); 

