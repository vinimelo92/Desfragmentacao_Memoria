/*
 * heap.h
 *
 *  Created on: 22 de abr de 2021
 *      Author: vinicius.passarella
 */

#ifndef SRC_HEAP_H_
#define SRC_HEAP_H_

#include <stdint.h>
#include <stddef.h>

#define HEAP_SIZE 0xEA60 // 60Kbytes

#define MAX_NUMBER_OBJ 10

#define MIN_ALLOC_SZ 4

#define MIN_WILDERNESS 0x0
#define MAX_WILDERNESS 0x1000000

#define BIN_COUNT 9
#define BIN_MAX_IDX (BIN_COUNT - 1)

typedef unsigned int uint;

typedef struct node_t {
    uint hole;
    uint size;
    struct node_t* next;
    struct node_t* prev;
} node_t;

typedef struct {
    node_t *header;
} footer_t;

typedef struct {
    node_t* head;
} bin_t;

typedef enum {false, true} bool;

typedef struct {
    long start;
    long end;
    bool active;
    bin_t *bins[BIN_COUNT];
} heap_t;

typedef struct {
	char *address_1;
	char *address_2;
	uint size;
} obj_t;

typedef struct {
  obj_t *object[MAX_NUMBER_OBJ];
  int totalObj;
} list_t;

void init_heap(heap_t *heap, long startAddress);

int *heap_alloc(heap_t *heap, size_t size);
void heap_free(heap_t *heap, char *address);
void heap_remove_object(heap_t *heap_1, heap_t *heap_2, list_t *lstObjects, obj_t *objectToBeRemoved);
uint expand(heap_t *heap, size_t sz);

uint get_bin_index(size_t sz);
void create_foot(node_t *head);
footer_t *get_foot(node_t *head);

node_t *get_wilderness(heap_t *heap);
obj_t create_object(heap_t *heap_1, heap_t *heap_2, int size);
void push(list_t *lstObjects, obj_t *obj);
void remove_object_from_list(list_t *lstObjects, obj_t *objectToBeRemoved);
void organize_objects(list_t *lstObjects);
void print_objects(list_t *lstObjects);
void switch_heap(heap_t *heap_1, heap_t *heap_2, list_t *lstObjects);
heap_t create_heap(long address);

#endif /* SRC_HEAP_H_ */
