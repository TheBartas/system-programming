#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/ipc.h>

#include "data.h"

char *_conntomem(char *file_name, struct stat *st) { // _connect_to_shm
    int fd = open(file_name, O_RDONLY); 

    if (fstat(fd, st) == -1) {
        fprintf(stderr, "[Error] cannot get file size. File <name> [%s]\n", file_name);
        return NULL;
    }

    char *mem_file = NULL;
    if ((mem_file = mmap(NULL, st->st_size, PROT_READ, MAP_SHARED, fd, 0)) == NULL) {
        fprintf(stderr, "Error: mmap file <name> [%s]\n", file_name);
        return NULL;
    }

    close(fd);
    return mem_file;
}

void snd_hellomsg(int id, hllmsg _hllmsg) {
    msgsnd(id, &_hllmsg, sizeof(_hllmsg) - sizeof(long), 0);
}

int main(int argc, char *argv[]) {
    const char *optstring = "k:t:";
    int t_flag = 0, k_flag = 0, ret, t = 0;
    key_t key;

    while ((ret = getopt(argc, argv, optstring)) != -1) {
        switch (ret) {
            case 't':t_flag = 1; t = atoi(optarg); break;
            case 'k':k_flag = 1; key = atoi(optarg); break;
            case '?': return -1;
            default: abort();
        }
    }

    printf("----------------------------------------------------\n");
    printf("Key:                   [%d]\n", key);
    printf("Number of tasks:       [%d]\n", t);
    printf("----------------------------------------------------\n");

    int r;

    int id = msgget(key, IPC_CREAT | 0600);
    strdt data;

    hllmsg _hllmsg = { .type = 4, .ready = true, .id = getpid()};
    snd_hellomsg(id, _hllmsg);
    printf("hello send!\n");

    r = msgrcv(id, &data, sizeof(data) - sizeof(long), 0, 0); // IPC_NOWAIT
    if(r < 0)
        return -1;

    bckmsg _bckmsg = {.type = 4, .found = false, .rec = true};
    msgsnd(id, &_bckmsg, sizeof(_bckmsg) - sizeof(long), 0);

    char *file_name = data.file_name;
    int data_offset = data.offset;
    int data_length = data.length;

    printf("Source file name: %s\n", file_name);
    printf("----------------------------------------------------\n");
    printf("Offset (begin index):       [%d]\n", data_offset);
    printf("Lenght (last index):        [%d]\n", data_length);
    printf("Hash:                       [TO DO]\n");
    printf("----------------------------------------------------\n");


    struct stat st;
    char *mem_file = _conntomem(file_name, &st);

    if (mem_file) 
        printf("Load to file correct!\n");
    else 
        printf("Faild to load file...!");

    FILE *in = fmemopen(&mem_file[data.offset], data.length, "r");
    char *buf = NULL;
    size_t size_buf = 0;
    int nlines = 0;

    while (getline(&buf, &size_buf, in) != -1) {
        printf("%s", buf);
    }

    free(buf);
    fclose(in);
}