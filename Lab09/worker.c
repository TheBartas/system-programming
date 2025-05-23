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
#include <signal.h>

#include "data.h"


static int new_s1 = 0;
static int old_s1 = 0;
static int id = -1;

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

void calc_range(char *mem_file, size_t fsize, int frt_prev, int scd_prev, int *s1, int *s2) {
    FILE *in = fmemopen(mem_file, fsize, "r");
    char *buf = NULL;
    size_t size_buf = 0;
    int nlines = 0;
    int size = 0;

    *s1 = 0;
    int len = 0;
    while (getline(&buf, &size_buf, in) != -1 && nlines < scd_prev) {
        if (nlines == frt_prev) {
            *s1 = size;
            size = 0;
        }
        
        size += strlen(buf);
        nlines++;
    }

    *s2 = size;
    fclose(in);
    free(buf);
}

void snd_hellomsg(int id, hllmsg _hllmsg) {
    msgsnd(id, &_hllmsg, sizeof(_hllmsg) - sizeof(long), 0);
}


void siginthndl(int sig) {
    printf("\t[Worker: %d] Something went wrong...I'm dying...\n", getpid());
    if (new_s1 > old_s1) printf("\t[Worker: %d] Reports: new first index \"s1\" is %d. [Last Report]\n", getpid(), new_s1);

    errmsg _errmsg = { .type = 10, .pid = getpid(), .nwline = (new_s1 > old_s1) ? new_s1 : -1};
    msgsnd(id, &_errmsg, sizeof(_errmsg) - sizeof(long), 0);

    exit(-1);
}

void set_sigint() {
    struct sigaction sigint;
    sigint.sa_handler = &siginthndl;
    sigint.sa_flags = 0;
    sigemptyset(&sigint.sa_mask); 
    sigaction(SIGINT, &sigint, NULL);
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
    set_sigint();

    int r;

    id = msgget(key, IPC_CREAT | 0600);

    // ------ send data ------

    hllmsg _hllmsg = { .type = 4, .ready = true, .id = getpid(), .tasks = t };
    snd_hellomsg(id, _hllmsg);
    printf("hello send!\n");

    
    // ------ get data ------

    strdt data;
    r = msgrcv(id, &data, sizeof(data) - sizeof(long), 1, 0); // IPC_NOWAIT
    if(r < 0)
        return -1;

    // ------ send if recive ------

    bckmsg _bckmsg = {.type = 2, .found = false, .rec = true};
    msgsnd(id, &_bckmsg, sizeof(_bckmsg) - sizeof(long), 0);


    char *file_name = data.file_name;
    int data_frt_idx = data.frt_idx;
    int data_scd_idx = data.scd_idx;
    old_s1 = data_frt_idx;

    printf("Source file name: %s\n", file_name);
    printf("----------------------------------------------------\n");
    printf("Offset (begin index):       [%d]\n", data_frt_idx);
    printf("Lenght (last index):        [%d]\n", data_scd_idx);
    printf("Hash:                       [TO DO]\n");
    printf("----------------------------------------------------\n");


    struct stat st;
    char *mem_file = _conntomem(file_name, &st);

    if (mem_file) 
        printf("Load to file correct!\n");
    else {
        fprintf(stderr, "Faild to load file <name> <%s>.\n", file_name);
        exit(-1);
    }



    int start_idx = 0, end_idx = 0;
    calc_range(mem_file, st.st_size, data_frt_idx, data_scd_idx, &start_idx, &end_idx);

    FILE *in = fmemopen(&mem_file[start_idx], end_idx, "r");
    char *buf = NULL;
    size_t size_buf = 0;
    int nlines = 0;

    int i = 0;

    bool is_found = false;
    while (getline(&buf, &size_buf, in) != -1) {
        sleep(2);
        new_s1++;
        printf("i = %d | %s", ++i, buf);

        if (is_found) {
            
        }
    }

    free(buf);
    fclose(in);
}