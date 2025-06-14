// PS IS 320 LAB06
// Bartłomiej Szewczyk
// sb53955@zut.edu.pl
#ifndef _LAB06
#define _LAB06

#define SUCCESS 0
#define ERROR_ALLOC_ZERO_SIZE -10
#define ERROR_MALLOC -4
#define ERROR_REALLOC -3
#define ERROR_CALLOC -2
#define ERROR_FREE -1

typedef struct node1d {
    void *inptr;
    size_t size;
    struct node1d * next;
} node1d_t;



void *mem_alloc(void *ptr, int size);
int mem_free(void *ptr);
void _show_data();

#endif