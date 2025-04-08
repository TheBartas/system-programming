#include <stdio.h>
#include <stdlib.h>
#include "lib.h"

extern int status_code;
extern int list_size;

int main() {
    int *arr = NULL;
    int size = 10;
    arr = mem_alloc(arr, size); 
    printf("Operation: %d\n", status_code);

    // for (int i =0; i < size ;i++) {
    //     arr[i] = i;
    // }
    // for (int i =0; i < size ;i++) {
    //     printf("arr[%d] = %d\n", i, arr[i]);
    // }
 
    char* arr2 = NULL;
    arr2 = mem_alloc(arr2, size);
    printf("Operation: %d\n", status_code);

    char* arr3 = NULL;
    arr3 = mem_alloc(arr3, size);
    printf("Operation: %d\n", status_code);

    _show_data();
    printf("List size (number of nodes): %d\n", list_size);

    size =  20;
    arr2 = mem_alloc(arr2, size);
    printf("Operation: %d\n", status_code);

    _show_data();
    printf("List size (number of nodes): %d\n", list_size);

    mem_free(arr);
    mem_free(arr2);
    mem_free(arr3);
    printf("Operation: %d\n", status_code);
    return 0;
}

