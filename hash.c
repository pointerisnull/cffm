#include "hash.h"
#include "data.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

void ht_new_entry(ht_entry *e) {
  e->next = NULL;
  e->dir = NULL;
}
/*FNV-1 hash function
uint64_t get_hash(char *string) {
  uint64_t fnv_offset = 0xcbf29ce484222325;
  uint64_t fnv_prime = 0x00000100000001B3;
  uint64_t hash = fnv_offset;
  int length = strlen(string);
  int i;
  for (i = 0; i < length; i++) {
    hash *= fnv_prime;
    hash ^= string[i];
  }
  return hash%HASHTABLE_SIZE;
} */

uint64_t get_hash(char *string) {
  return strlen(string)%1;
}

void ht_insert(Table *ht, void *element, unsigned index) {
  if (ht->entries[index].dir == NULL) 
    ht->entries[index].dir = element;
  else {
    ht_entry *current = &ht->entries[index];
    while (current->next != NULL) {
      current = current->next;
    }
    ht_entry *new = malloc(sizeof(ht_entry));
    ht_new_entry(new);
    new->dir = element;
    current->next = new;
    ht->collisions++;
  }
}

int ht_delete_element(Table *ht, char *string) {
  unsigned int index = get_hash(string);
  if (ht->entries[index].dir == NULL)  return 1;
  Directory *dir = ht->entries[index].dir;
  if (ht->entries[index].next == NULL) {
    /*if directory is only one at index (most likely)*/
    ht->entries[index].dir = NULL;
  } else if (strcmp(dir->path, string) == 0) {
    /*if directory is at the head */
    ht_entry *next = ht->entries[index].next;
    ht->entries[index].dir = next->dir;
    ht->entries[index].next = next->next;
    free(next);

  } else {
    /*if directory is in the middle of a chain*/
    ht_entry *prev = &ht->entries[index];
    ht_entry *current = ht->entries[index].next;
    ht_entry *next = current->next;
    dir = current->dir;
    int found = 1;
    while(found != 0) {
      found = strcmp(dir->path, string);
      if (found == 0) break;
      prev = current;
      current = current->next;
      if (current == NULL) break;
      dir = current->dir;
      next = current->next;
    }
    if (found != 0) return 1;
    current->dir = NULL;
    prev->next = next;
    free(current);

  }
  return 0;

}

void free_chain(ht_entry *head) {
  if (head->next == NULL) return;
  else free_chain(head->next);
  free(head->next);
}

void ht_free(Table *ht) {
  int i;
  for (i = 0; i < HASHTABLE_SIZE; i++) {
    if (ht->entries[i].next != NULL)
      free_chain(&ht->entries[i]);
  }
  free(ht->entries);
}

void ht_init(Table *ht) {
  ht->collisions = 0;
  ht->entries = malloc(sizeof(ht_entry)*HASHTABLE_SIZE);
  int i;
  for(i = 0; i < HASHTABLE_SIZE; i++) {
    ht_new_entry(&ht->entries[i]);
  }
}
