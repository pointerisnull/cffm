#ifndef HASH_H
#define HASH_H

#define HASHTABLE_SIZE 1024

#include <stdint.h>

typedef struct ht_entry ht_entry;
typedef struct Table Table;

struct ht_entry {
  void *dir; 
  ht_entry *next;
};

struct Table {
  int collisions;
  ht_entry *entries;
};

uint64_t get_hash(char *string);
void ht_init(Table *ht);
void ht_insert(Table *ht, void *element, unsigned index);
int ht_delete_element(Table *ht, char *string);
void ht_free(Table *ht);

#endif
