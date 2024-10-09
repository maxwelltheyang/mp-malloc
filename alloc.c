/**
 * Malloc
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct _metadata_t {
  unsigned int size;     // The size of the memory block.
  unsigned char isUsed;  // 0 if the block is free; 1 if the block is used.
  struct _metadata_t* prev;
  struct _metadata_t* next;
} metadata_t;

metadata_t* startOfHeap = NULL;
void* endOfHeap = NULL;
metadata_t* head = NULL;
//int counter = 0;

void print_heap() {
  metadata_t *curMeta = startOfHeap;
  printf("-- Start of Heap (%p) --\n", startOfHeap);
  while ((void *)curMeta < endOfHeap && curMeta) {   // While we're before the end of the heap...
    printf("metadata for memory %p: (%p, size=%d, isUsed=%d)\n", 
        (void *)curMeta + sizeof(metadata_t), 
        curMeta, 
        curMeta->size, 
        curMeta->isUsed
    );
    curMeta = (void *)curMeta + curMeta->size + sizeof(metadata_t);
  }
  printf("-- End of Heap (%p) --\n\n", endOfHeap);
}

void delete_block(metadata_t* data) {
  if (!data || !head) {return;}
  if (data == head) {
    head = data->next;
    //head->prev = NULL;
  }
  if (data->next != NULL) {
    data->next->prev = data->prev;
  } 
  if (data->prev != NULL) {
    data->prev->next = data->next;
  }
  
}

void insert_block(metadata_t* data) {
  if (!data) {return;}
  metadata_t* temp = head;
  metadata_t* prev_temp = NULL;
  if (head == NULL) {
    head = data;
    head->prev = NULL;
    head->next = NULL;
    return;
  }
  while (temp != NULL && (temp < data)) {
    prev_temp = temp;
    temp = temp->next;
  }
  if (temp == head) {
    data->next = head;
    data->prev = NULL;
    head->prev = data;
    head = data;
  } else {
    data->next = temp;
    data->prev = prev_temp;
    prev_temp->next = data;
    if (temp != NULL) {
      temp->prev = data;
    }
  }
}

void combine_block(metadata_t* meta) {
  if (!meta) {return;}
  metadata_t* next = meta->next;
  if (next != NULL && next->isUsed == 0 && (metadata_t*)((char*)meta + meta->size + sizeof(metadata_t)) == next) {
    meta->size = meta->size + next->size + sizeof(metadata_t);
    meta->next = next->next;
    if (next->next != NULL) {
      next->next->prev = meta;
    }
    delete_block(next);
  }

  metadata_t* prev = meta->prev;
  if (prev != NULL && prev->isUsed == 0 && (metadata_t*)((char*)prev + prev->size + sizeof(metadata_t)) == meta) {
    prev->size = prev->size + meta->size + sizeof(metadata_t);
    prev->next = meta->next;
    if (meta->next != NULL) {
      meta->next->prev = prev;
    }
    delete_block(meta);
  }
}
/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
  if (num == 0 || size == 0) {
    return NULL;
  }
  void* ptr = malloc(num * size);
  if (ptr == NULL) {
    return ptr;
  }
  memset(ptr, 0, num * size);
  return ptr;
}


/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
  metadata_t* temp = head;
  while (temp != NULL) {
    if (temp->size >= size && temp->isUsed == 0) { //second statement is a failsafe
      //printf("should not exec\n");
      int diff = temp->size - size - sizeof(metadata_t);
      //printf("%d\n", diff);
      if (diff > 1) {
        temp->size = size;
        temp->isUsed = 1;
        //void* new_loc = (void*)((char*)temp + sizeof(metadata_t) + size);
        metadata_t* new_space = (metadata_t *)((char *)(temp) + sizeof(metadata_t) + size);
        new_space->size = diff;
        new_space->isUsed = 0;
        insert_block(new_space);
        // combine(new_space);
      }
      temp->isUsed = 1;
      //temp->size = size;
      delete_block(temp);
      return (void*)((char*)temp + sizeof(metadata_t));
    }
    //printf("stuck :p\n");
    temp = temp->next;
  }
  metadata_t* new_data = sbrk(sizeof(metadata_t) + size);
  if (new_data == (void *)-1) {
    return NULL;
  }
  new_data->size = size;
  new_data->isUsed = 1;
  if (startOfHeap == NULL) {
    startOfHeap = new_data;
  }
  endOfHeap = sbrk(0);
  return (void*)((char*)new_data + sizeof(metadata_t));
}


/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
  if (ptr == NULL) {
    return;
  }
  metadata_t *meta = (metadata_t *)((char *)ptr - sizeof(metadata_t));
  meta->isUsed = 0;
  insert_block(meta);
  combine_block(meta);
}


/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
  if (ptr == NULL) {
    return malloc(size);
  }
  if (size == 0) {
    free(ptr);
    return NULL;
  }
  metadata_t* curr = (metadata_t *)((char*)ptr - sizeof(metadata_t));
  size_t data_size = curr->size;
  if (size <= data_size) {
    return ptr;
  } 
  else {
    void* new_loc = malloc(size);
    if (new_loc == NULL) {
      return NULL;
    }
    memcpy(new_loc, ptr, data_size);
    free(ptr);
    return new_loc;
  }
}
