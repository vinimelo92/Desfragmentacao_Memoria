#include "heap.h"
#include "main.h"
#include "llist.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

uint offset = 8;
static char buf[256] = {0};
static uint overhead = (uint)(sizeof(footer_t) + sizeof(node_t));

void init_heap(heap_t *heap, long startAddress) {
    node_t *init_region = (node_t *) startAddress;
    init_region->hole = 1;
    init_region->size = (HEAP_SIZE) - sizeof(node_t) - sizeof(footer_t);

    create_foot(init_region);

    add_node(heap->bins[get_bin_index(init_region->size)], init_region);

    heap->start = startAddress;
    heap->end   = startAddress + HEAP_SIZE;
}

int *heap_alloc(heap_t *heap, size_t size) {
    uint index = get_bin_index(size);
    bin_t *temp = (bin_t *) heap->bins[index];

    node_t *found = get_best_fit(temp, size);
    while (found == NULL) {
        if (index + 1 >= BIN_COUNT)
            return NULL;

        temp = heap->bins[++index];
        found = get_best_fit(temp, size);
    }

    if((found->size - size) > (overhead + MIN_ALLOC_SZ)){
        node_t *split = (node_t *) (((char *) found + sizeof(node_t) + sizeof(footer_t)) + size);
        split->size = found->size - (uint)size - (uint)sizeof(node_t) - (uint)sizeof(footer_t);
        split->hole = 1;

        create_foot(split);

        uint new_idx = get_bin_index(split->size);

        add_node(heap->bins[new_idx], split);

        found->size = size;
        create_foot(found);
    }

    found->hole = 0;
    remove_node(heap->bins[index], found);

    node_t *wild = get_wilderness(heap);
    if(wild->size < MIN_WILDERNESS){
        uint success = expand(heap, 0x1000);
        if(success == 0){
            return NULL;
        }
    }

    found->prev = NULL;
    found->next = NULL;
    return (int *)&found->next;
}

void heap_free(heap_t *heap, char *address){
	bin_t *list;
    footer_t *new_foot, *old_foot;

    node_t *head = (node_t *) ((char *) address - offset);
    if(head == (node_t *) (uintptr_t) heap->start){
        head->hole = 1;
        add_node(heap->bins[get_bin_index(head->size)], head);
        return;
    }

    node_t *next = (node_t *) ((char *) get_foot(head) + sizeof(footer_t));
    footer_t *f = (footer_t *) ((char *) head - sizeof(footer_t));
    node_t *prev = f->header;

    if(prev->hole){
        list = heap->bins[get_bin_index(prev->size)];
        remove_node(list, prev);

        prev->size += overhead + head->size;
        new_foot = get_foot(head);
        new_foot->header = prev;

        head = prev;
    }

    if(next->hole){
        list = heap->bins[get_bin_index(next->size)];
        remove_node(list, next);

        head->size += overhead + next->size;

        old_foot = get_foot(next);
        old_foot->header = 0;
        next->size = 0;
        next->hole = 0;

        new_foot = get_foot(head);
        new_foot->header = head;
    }

    head->hole = 1;
    add_node(heap->bins[get_bin_index(head->size)], head);
}

void heap_remove_object(heap_t *heap_1, heap_t *heap_2, list_t *lstObjects, obj_t *objectToBeRemoved) {
	heap_t *active_heap;
    char *address;

    if(((heap_1->active == true) && (heap_2->active == true)) || ((heap_1->active == false) && (heap_2->active == false))){
    	// Issue on heaps. Both heaps are activated or they are off.
    	return;
	}

    if(heap_1->active == true){
    	active_heap = heap_1;
    	address = objectToBeRemoved->address_1;
	} else {
		active_heap = heap_2;
		address = objectToBeRemoved->address_2;
	}

    heap_free(active_heap, address);

    // print objects from the list
    print_objects(lstObjects);

    // Remove the object from the list
    remove_object_from_list(lstObjects, objectToBeRemoved);

    // Organize objects inside the list
    organize_objects(lstObjects);

    // print objects from the list
    print_objects(lstObjects);

    // Switch heap to avoid fragmentation.
    switch_heap(heap_1, heap_2, lstObjects);

    print_objects(lstObjects);
}

uint expand(heap_t *heap, size_t sz) {
    return 0;
}

uint get_bin_index(size_t sz) {
    uint index = 0;
    sz = sz < 4 ? 4 : sz;

    while(sz >>= 1) index++;
	index -= 2;

    if(index > BIN_MAX_IDX) index = BIN_MAX_IDX;
    return index;
}

void create_foot(node_t *head) {
    footer_t *foot = get_foot(head);
    foot->header = head;
}

footer_t *get_foot(node_t *node) {
    return (footer_t *) ((char *) node + sizeof(node_t) + node->size);
}

node_t *get_wilderness(heap_t *heap) {
    footer_t *wild_foot = (footer_t *) ((char *) heap->end - sizeof(footer_t));
    return wild_foot->header;
}

obj_t create_object(heap_t *heap_1, heap_t *heap_2, int size) {
	obj_t object;
	if(heap_1->active == true){
		object.address_1 = (char*) heap_alloc(heap_1, size);
		object.address_2 = (char*)1;
		strcpy(object.address_2, "");
	} else{
		object.address_1 = (char*)1;
		strcpy(object.address_1, "");
		object.address_2 = (char*) heap_alloc(heap_2, size);
	}
	object.size = size;
	return object;
}

void remove_object_from_list(list_t *lstObjects, obj_t *objectToBeRemoved) {
	strcpy(buf,"\nremove_object_from_list\n\n\r");
	SerialTransmit (buf, strlen(buf));

    for(int i=0; i<MAX_NUMBER_OBJ; i++){
    	if(lstObjects->object[i] == objectToBeRemoved){
    		lstObjects->object[i] = NULL;
    		lstObjects->totalObj = lstObjects->totalObj - 1;
			break;
		}
	}
}

void push(list_t *lstObjects, obj_t *obj) {
	lstObjects->totalObj += 1;
	lstObjects->object[lstObjects->totalObj] = obj;
}

void organize_objects(list_t *lstObjects) {
	strcpy(buf,"\nOrganize_objects\n\n\r");
	SerialTransmit (buf, strlen(buf));

	for(int i=0; i<MAX_NUMBER_OBJ; i++){
		if(lstObjects->object[i] == NULL){
			for (int j=i+1; j<MAX_NUMBER_OBJ; j++){
				if(lstObjects->object[j] != NULL){
					lstObjects->object[i] = lstObjects->object[j];
					lstObjects->object[j] = NULL;
					break;
				}
			}
		}
	}
}

void print_objects(list_t *lstObjects) {
	strcpy(buf, "\n******************** print_objects ********************\n\r");
	SerialTransmit (buf, strlen(buf));
	sprintf(buf, "totalObj:[%d]\n\r", lstObjects->totalObj);
	SerialTransmit (buf, strlen(buf));
	for(int i=0; i<=lstObjects->totalObj; i++){
		sprintf(buf, "Object[%d] Size:[%d] --> address_1:[%X] Content:[%s] / address_2:[%X] Content:[%s]\n\r",
						i, (lstObjects->object[i])->size,
						(unsigned int)(lstObjects->object[i])->address_1, (lstObjects->object[i])->address_1,
						(unsigned int)(lstObjects->object[i])->address_2, (lstObjects->object[i])->address_2);
		SerialTransmit (buf, strlen(buf));
	}
	strcpy(buf, "*******************************************************\n\n\r");
	SerialTransmit (buf, strlen(buf));
}

void switch_heap(heap_t *heap_1, heap_t *heap_2, list_t *lstObjects) {
	strcpy(buf,"\nSwitch_heap\n\n\r");
	SerialTransmit (buf, strlen(buf));

    for(int i=0; i<=lstObjects->totalObj; i++){
    	if(i==MAX_NUMBER_OBJ)
    		break;

    	if(heap_1->active == true){
	    	(lstObjects->object[i])->address_2 = (char*) heap_alloc(heap_2, (lstObjects->object[i])->size);
    		memcpy((lstObjects->object[i])->address_2, (lstObjects->object[i])->address_1, strlen((lstObjects->object[i])->address_1));
    		heap_free(heap_1, (lstObjects->object[i])->address_1);
    		(lstObjects->object[i])->address_1 = (char*)1;
		} else {
			(lstObjects->object[i])->address_1 = (char*) heap_alloc(heap_1, (lstObjects->object[i])->size);
    		memcpy((lstObjects->object[i])->address_1, (lstObjects->object[i])->address_2, strlen((lstObjects->object[i])->address_2));
    		heap_free(heap_2, (lstObjects->object[i])->address_2);
			(lstObjects->object[i])->address_2 = (char*)1;
		}
	}

	if(heap_1->active == true){
    	heap_1->active = false;
		heap_2->active = true;
		memset(&heap_1, 0, sizeof(heap_1));
	} else{
		heap_1->active = true;
		heap_2->active = false;
		memset(&heap_2, 0, sizeof(heap_2));
	}
}

heap_t create_heap(long address) {
	heap_t heap;
    memset(&heap, 0, sizeof(heap_t));

    for(int i = 0; i < BIN_COUNT; i++){
        heap.bins[i] = malloc(sizeof(bin_t));
        memset(heap.bins[i], 0, sizeof(bin_t));
    }

    init_heap(&heap, address);
    return heap;
}

