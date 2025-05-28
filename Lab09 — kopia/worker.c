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
#include <crypt.h>

#include "data.h"

static int id = -1;
static int pgid= -1; // if (not)found password 
static FILE *in = NULL;
static char *buf = NULL;
static char *mem_file = NULL;
static struct stat st;

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

void snd_hellomsg(int id, int t) {
    hllmsg _hllmsg = {0};
    _hllmsg.type = TYPE_HLLMSG_QUE;
    _hllmsg.ready = true;
    _hllmsg.id = getpid();
    _hllmsg.tasks = t;
    msgsnd(id, &_hllmsg, sizeof(_hllmsg) - sizeof(long), 0);
}


void siginthndl(int sig) {
    if (mem_file) {
        munmap(mem_file, st.st_size);
        printf("[Worker] \'mem_file\' munmaped.\n");
    }

    if (in) {
        fclose(in);
        printf("[Worker] \'*in\' closed.\n");
    }

    if (buf) {
        free(buf);
        printf("[Worker] \'buf\' closed.\n");
    }
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

    set_sigint();

    int r;

    id = msgget(key, IPC_CREAT | 0600);

    key_t pkey = ftok("worker.c", 'z');
    pgid= msgget(pkey, IPC_CREAT | 0600);

    printf("----------------------------------------------------\n");
    printf("Key:                   [%d]\n", key);
    printf("Progress Key:          [%d]\n", pkey);
    printf("Number of tasks:       [%d]\n", t);
    printf("Worker PID:            [%d]\n", getpid());
    printf("----------------------------------------------------\n");

    // ------ send data ------
    
    snd_hellomsg(id, t);
    
    // ------ get data ------

    strdt data;
    r = msgrcv(id, &data, sizeof(data) - sizeof(long), 1, 0); // IPC_NOWAIT
    if(r < 0)
        return -1;

    // ------ send if recive ------

    bckmsg _bckmsg = {0}; 
    _bckmsg.type = TYPE_BCKMSG_QUE;
    _bckmsg.rec = true;
    _bckmsg.found = false;
    _bckmsg.pid = getpid();
    _bckmsg.mtasks = -1;
    msgsnd(id, &_bckmsg, sizeof(_bckmsg) - sizeof(long), 0);


    char *file_name = data.file_name;
    int data_frt_idx = data.frt_idx;
    int data_scd_idx = data.scd_idx;
    printf("\n\n----------------------------------------------------\n");
    printf("Source file name: %s\n", file_name);
    printf("Offset (begin index):       [%d]\n", data_frt_idx);
    printf("Lenght (last index):        [%d]\n", data_scd_idx);
    printf("Hash:                       [TO DO: %s]\n", data.hash);
    printf("----------------------------------------------------\n");

    mem_file = _conntomem(file_name, &st);

    if (mem_file) 
        printf("[Worker / Info] Load to file correct!\n");
    else {
        fprintf(stderr, "[Worker / Error] Faild to load file <name> <%s>.\n", file_name);
        exit(-1);
    }


    int start_idx = 0, end_idx = 0;
    calc_range(mem_file, st.st_size, data_frt_idx, data_scd_idx, &start_idx, &end_idx);

    in = fmemopen(&mem_file[start_idx], end_idx, "r");

    size_t size_buf = 0;
    int nlines = 0;

    int i = 0, m = 350;
    struct crypt_data _crypt_data;
    char *encrpt = NULL; 

    pgrmsg _pgrmsg = {0};
    _pgrmsg.type = TYPE_PRGMSG_QUE;
    bool is_found = false;
    while (getline(&buf, &size_buf, in) != -1) {
        usleep(1000);
        buf[strcspn(buf, "\r\n")] = 0; 

        // if (strcmp(buf, data.hash) == 0) {
        //     is_found = true;
        //     break;
        // }

        if ((encrpt = crypt_r(buf, "$6$5MfvmFOaDU$", &_crypt_data)) != NULL) {
            if (strcmp(encrpt, data.hash) == 0) {
                is_found = true;
                break;
            }
        }

        if (++i % m == 0) {
            _pgrmsg.ndone = m;
            msgsnd(pgid, &_pgrmsg, sizeof(_pgrmsg) - sizeof(long), 0);
        }
    }

    if (is_found)  {
        printf("[Worker / %d] Found!\n", getpid());
        _bckmsg.found = true;
    }

    _bckmsg.type = TYPE_FNDEND_BCKMSG_QUE;
    _bckmsg.mtasks = data.tasks;
    msgsnd(id, &_bckmsg, sizeof(_bckmsg) - sizeof(long), 0);
    
    free(buf);
    if (mem_file) {
        munmap(mem_file, st.st_size);
    }
    fclose(in);
}