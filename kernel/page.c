#include <stdint.h>
#include <stdbool.h>

// create a page entry
// should be 64 bits
typedef struct page_table_entry {
  bool present : 1;
  bool writable : 1;
  bool kernel : 1;
  uint16_t unused : 9;
  uint64_t address : 51;
  bool no_execute : 1;
} __attribute__((packed)) pt_entry_t;

// 52 - 12 = 40