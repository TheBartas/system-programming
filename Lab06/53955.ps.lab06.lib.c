// PS IS 320 LAB06
// Bartłomiej Szewczyk
// sb53955@zut.edu.pl
#include <stdio.h>
#include <stdlib.h>
#include "53955.ps.lab06.lib.h"

node1d_t * head = NULL;
node1d_t * tail = NULL;
int list_size = 0;
int status_code;

void _clean_all() {
    node1d_t * current = head;

    while (current != NULL) {
        node1d_t * temp = current->next;
        free(current->inptr);
        free(current);
        current = temp;
    }
    head = NULL;
    tail = NULL;
    list_size = 0;
}


void _add_node(void *ptr, size_t size) {
    node1d_t *new_node = NULL; 
    if ((new_node = malloc(sizeof(node1d_t))) == NULL) {
        status_code = ERROR_MALLOC;
        return;
    } 
    new_node->inptr = ptr;
    new_node->size = size;
    new_node->next = NULL;

    if(head == NULL) {
        head = new_node;
    }

    if(tail != NULL) {
        tail->next = new_node;
    }
    tail = new_node;
    list_size++;
}

node1d_t *_find(void *ptr) {
    node1d_t *current = head;
    while(current != NULL) {
        if (current->inptr == ptr) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void _remove_node(void *ptr) {
    node1d_t *prev = NULL;
    node1d_t *current = head;

    while(current!=NULL) {
        if (current->inptr == ptr) {
            if (prev == NULL) {
                head = current->next;

            } else {
                prev->next = current->next;
            }

            if (current == tail) {
                tail = prev;
            }
            free(current);
            list_size--;
            return;
        }

        prev = current;
        current = current->next;
    }

}

void _show_data() {
    node1d_t *current = head;
    int i = 0;
    printf("List length: [%d]\n", list_size);
    while(current != NULL) {
        i++;
        printf("Addr [%d]: %p (%ld)\n", i, current->inptr, current->size);
        current = current->next;
    }
}

void __attribute__((constructor)) _init_list() { 
    atexit(_clean_all);
}

void *mem_alloc(void *ptr, int size) {
    if (size == 0) {
        status_code = ERROR_ALLOC_ZERO_SIZE;
        return NULL;
    } 
    
    if (ptr == NULL) {
        void *cptr = calloc(1, size);

        if (cptr != NULL) {
            _add_node(cptr, size);
            status_code = SUCCESS;
            return cptr;
        }

        status_code = ERROR_CALLOC;
        return NULL;

    } else {
        node1d_t * node = NULL;      

        if ((node = _find(ptr)) != NULL) {
            void *rlptr = NULL;

            if ((rlptr = realloc(ptr, size)) == NULL) {
                status_code = ERROR_REALLOC;
                return NULL;
            }

            node->inptr = rlptr;
            node->size = size;
            return rlptr;

        } 

        return NULL;
    }

    return NULL;
}

int mem_free(void *ptr) {
    if (ptr != NULL) {
        _remove_node(ptr);
        free(ptr);
        status_code = SUCCESS;
        return NULL;
    } 
    status_code = ERROR_FREE;
    return NULL;
}
