#include <stdio.h>
#include <stdlib.h>


int main(int argc, char * argv[]) {
    int n = atoi(argv[1]);
    int ratio = atoi(argv[2]);
    int arr[n];
    for (int i = 0; i < n; i++) {
        arr[i] = n * ratio + i * (n % ratio);
        //printf("%d\n", arr[i]);
    }
    return 0;
}