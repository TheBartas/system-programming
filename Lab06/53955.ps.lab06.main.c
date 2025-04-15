// PS IS 320 LAB06
// Bartłomiej Szewczyk
// sb53955@zut.edu.pl
#include <stdio.h>
#include <stdlib.h>
#include "53955.ps.lab06.lib.h"

extern int status_code;
// extern void _auto_clean();

void test_1(int size) {
    printf("\n---- Run Test 1 ----\n");
    int *testa = NULL;
    testa = mem_alloc(testa, size);
    printf("Test A status code: %d\n", status_code); // 0

    mem_free(testa);
    printf("Free test A status code: %d\n", status_code); // 0
}

void test_2(int size) {
    printf("\n---- Run Test 2 ----\n");
    int *testa = NULL;
    testa = mem_alloc(testa, size);
    printf("Test A status code: %d\n", status_code); // 0

    char *testb = NULL;
    testb = mem_alloc(testb, size);
    printf("Test B status code: %d\n", status_code); // 0

    mem_free(testa);
    printf("Free test A status code: %d\n", status_code); // 0
    mem_free(testb);
    printf("Free test B status code: %d\n", status_code); // 0
}

void test_3(int size) {
    printf("\n---- Run Test 3 ----\n");
    char *testa = NULL;

    mem_free(testa);
    printf("Free test A status code: %d\n", status_code); // -1
}

void test_4(int size) {
    printf("\n---- Run Test 4 ----\n");
    size *= -1;

    char *testa = NULL;
    testa = mem_alloc(testa, size);
    printf("Test A status code: %d\n", status_code); // -10 / -2

    mem_free(testa);
    printf("Free test A status code: %d\n", status_code); // -1
}

void test_5(int size) {
    printf("\n---- Run Test 5 ----\n");
    char *testa = NULL;
    testa = mem_alloc(testa, size);
    printf("Test A status code: %d\n", status_code); // -0

    size *= -4;

    char *testb = NULL;
    testb = mem_alloc(testb, size);
    printf("Test B status code: %d\n", status_code); // -2


    mem_free(testa);
    printf("Free test A status code: %d\n", status_code); // 0
    mem_free(testb);
    printf("Free test A status code: %d\n", status_code); // -1
}

void test_6(int size) {
    printf("\n---- Run Test 6 ----\n");
    char * testa = NULL;
    testa = mem_alloc(testa, size);
    printf("Test A status code: %d\n", status_code); // 0

    testa = mem_alloc(testa, 2* size);
    printf("Test A (resize) status code: %d\n", status_code); // 0

    mem_free(testa);
    printf("Free test A status code: %d\n", status_code); // 0
}

void test_7(int size) {
    printf("\n---- Run Test 7 ----\n");
    char * testa = NULL;
    testa = mem_alloc(testa, size);
    printf("Test A status code: %d\n", status_code);

    testa = mem_alloc(testa, -2* size);
    printf("Test A (resize) status code: %d\n", status_code); 

    mem_free(testa);
    printf("Free test A status code: %d\n", status_code);
}




void test_opt(int i, int size) {
    switch (i) {
        case 1:
            test_1(size);
            break;
        case 2:
            test_2(size);
            break;
        case 3:
            test_3(size);   
            break;
        case 4:
            test_4(size);
            break;
        case 5:
            test_5(size);
            break;
        case 6:
            test_6(size);
            break;
        case 7:
            test_7(size);
            break;
    }
}

void test_run_all(int size) {
    for (int i = 1; i <= 7; i++){
        test_opt(i, size);
    }
}


int main() {
    // _auto_clean();
    int size = 10;
    // test_1(size);
    // test_opt(6, size);
    test_run_all(size);
}

