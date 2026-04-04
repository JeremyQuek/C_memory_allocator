/*
 * mm_alloc.c
 */

#include "mm_alloc.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef struct malloc_block{
  size_t size_bytes;
  bool used;
  struct malloc_block *next;
  struct malloc_block *prev;

  char data[];
} malloc_block;

typedef struct {
  malloc_block *head;
  malloc_block *tail;
  size_t num_blocks;
} linked_list;

size_t MIN_BLOCK_SIZE = sizeof(malloc_block) + 8;
linked_list free_list;
linked_list malloc_list;


void list_remove(malloc_block* block, linked_list *ll) {
  malloc_block* prev_block = block->prev;
  malloc_block* next_block = block->next;
  if (prev_block) prev_block->next = next_block;
  if (next_block) next_block->prev = prev_block;

  if (block==ll->head && block == ll->tail) {
    ll->head = NULL;
    ll->tail = NULL;
  } else if (block==ll->head) {
    ll->head = next_block;
  } else if (block == ll->tail) {
    ll->tail = prev_block;
  }

  block->next = NULL;
  block->prev = NULL;
  ll->num_blocks--;
}

void list_push_back(malloc_block* block, linked_list *ll) {
  if (ll->num_blocks==0) {
    ll->head = block;
    ll->tail = block;
  }
  else {
    ll->tail->next= block;
    block->prev = ll->tail;
    ll->tail = block;
  }
  ll->num_blocks++;
}

void list_sorted_insert(malloc_block* block, linked_list *ll) {
  if (ll->num_blocks==0) {
    list_push_back(block, ll);
    return;
  }

  malloc_block *insert_loc;
  for (insert_loc = ll->head; insert_loc!=NULL; insert_loc=insert_loc->next) {
    if (insert_loc>block) {
      malloc_block* prev_block = insert_loc->prev;

      if (prev_block) {
        prev_block->next = block;
      } else {
        ll->head = block; 
      }
      block->prev = prev_block;

      block->next = insert_loc;
      insert_loc->prev = block;

      ll->num_blocks++;
      return;
    }
  }
  list_push_back(block, ll);
}

void* mm_malloc(size_t size) {
  if (size==0) {
    return NULL;
  }

  size_t total_size = size + sizeof(malloc_block);
  void* return_ptr = NULL;

  // Case 1: Free list
  if (free_list.num_blocks!=0) {
    malloc_block *free_block;
    for (free_block = free_list.head; free_block!=NULL; free_block=free_block->next) {
      if (free_block->size_bytes>=total_size) {
        list_remove(free_block, &free_list);
        if (free_block->size_bytes-total_size>MIN_BLOCK_SIZE) {

          char* boundary = (char*)free_block + total_size;
          malloc_block *new_free_block = (malloc_block *) boundary;

          new_free_block->size_bytes= (size_t)((char*)free_block + free_block->size_bytes - boundary);
          new_free_block->used = false;
          new_free_block->next = NULL;
          new_free_block->prev = NULL;
          free_block->size_bytes = total_size;
          list_sorted_insert(new_free_block, &free_list);
        }
        list_push_back(free_block, &malloc_list);
        free_block->used = true;
        return_ptr = (void*)free_block->data;
        break;
      }
    }
  }
  
  // Case 2: No free list
  if (!return_ptr) {
    void* pointer = sbrk(total_size);
    if (pointer == (void*)-1) {
      return NULL;
    }

    malloc_block *new_block = (malloc_block *)pointer;
    new_block->size_bytes=total_size;
    new_block->used = true;
    new_block->next = NULL;
    new_block->prev = NULL;

    list_push_back(new_block, &malloc_list);
    return_ptr = (void*) new_block->data;
  }
  return return_ptr;
}


bool undefined_ptr(void*ptr , linked_list* ll) {
  malloc_block *temp;
  for (temp = ll->head; temp!=NULL; temp=temp->next) {
    if ((void*)temp->data == ptr){
      return false;
    }
  } 
  return true;
}

void mm_free(void* ptr) {
  if (ptr==NULL) {
    return;
  }
  // Undefined pointer
  if (undefined_ptr(ptr, &malloc_list)) {
    return ;
  }

  malloc_block * remove_block = (malloc_block *)((char *)ptr - sizeof(malloc_block));
  list_remove(remove_block, &malloc_list);
  list_sorted_insert(remove_block, &free_list);

  malloc_block * cur = remove_block;
  char* next_vaddr = (char*)cur + cur->size_bytes;

  // Merged forward
  while (cur->next && (malloc_block*)next_vaddr==cur->next) {
    malloc_block * next_block = cur->next;
    list_remove(next_block, &free_list);
    cur->size_bytes+=next_block->size_bytes;
    next_vaddr = (char*)cur + cur->size_bytes;
  }

  // Merge backward
  while (cur->prev && (char*)cur->prev + cur->prev->size_bytes == (char*)cur) {
      malloc_block *prev_block = cur->prev;
      prev_block->size_bytes += cur->size_bytes;
      list_remove(cur, &free_list);
      cur = prev_block;
  }
}

void* mm_realloc(void* ptr, size_t size) {
  if (ptr == NULL) return mm_malloc(size);
  if (size == 0) {
    mm_free(ptr);
    return NULL;
  }

  void* new_ptr = mm_malloc(size);
  if (new_ptr == NULL) return NULL;

  malloc_block* old_block = (malloc_block*)((char*)ptr - sizeof(malloc_block));
  size_t old_data_size = old_block->size_bytes - sizeof(malloc_block);
  size_t copy_size = old_data_size < size ? old_data_size : size;

  memcpy(new_ptr, ptr, copy_size);
  mm_free(ptr);
  return new_ptr;
}
