#include "thread.h"

int _compare(const void *t1, const void *t2) {
    const thread_t * arg1 = (const thread_t*)t1;
    const thread_t * arg2 = (const thread_t*)t2;

    if (arg1->ttl < arg2->ttl) return -1;
    if (arg1->ttl > arg2->ttl) return 1;
    return 0;
}

void * thread_alloc(int n) {
    thread_t * ptr = NULL;
    if((ptr = malloc(sizeof * ptr * n)) == NULL) {
        fprintf(stderr, "Error <malloc> at thread_alloc [%d]", ERROR_MALLOC);
        return NULL;
    };
    return ptr;
}

void thread_show(thread_t * _thread, int n) {
    if (_thread != NULL) {
        for (int i = 0;i<n;i++) {
            printf("\t\tttl: %d | thread id: %ld\n", _thread[i].ttl, _thread[i].thread_id);
        }
    }
}

void thread_sort(thread_t * _thread, int n) {
    qsort(_thread, n, sizeof(* _thread), _compare);
}   

void thread_calc_time(thread_t * _thread, int n) {
    for (int i = n - 1; i > 0; i--) {
        _thread[i].ttl = _thread[i].ttl - _thread[i-1].ttl;
    }
}