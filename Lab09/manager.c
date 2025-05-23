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
#include <sys/types.h>
#include <signal.h>

#include "data.h"

#define ERROR_NOARG -2


volatile sig_atomic_t timed_out = 0;
static char *mem_file = NULL;
static struct stat st;
static blckinfo *arr_infot = NULL;
static int id = -1;

// ipcs -q
// ipcrm -q <id> 


// Worker wysyła informacje w momencie pojawienia się, następnie 

char *_ldfltomem(char *file_name, struct stat *st) { //_load_file_to_memory
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

void prpr_file(char *mem_file, size_t fsize, int t, int *get_nlines, int *get_regbl_len, int *get_lastbl_len) { 
    FILE *in = fmemopen(mem_file, fsize, "r");
    char *buf = NULL;
    size_t size_buf = 0;

    while (getline(&buf, &size_buf, in) != -1) {
        (*get_nlines)++;
    }

    int last_offset = *get_nlines % t;
    *get_regbl_len = (*get_nlines - last_offset) / t;
    *get_lastbl_len = last_offset + *get_regbl_len;

    free(buf);
    fclose(in);
}

void queue_create(blckinfo *_blckinfo, int t, int regblc_nlines, int lastblc_nlines) {
    int idx = 0;
    int offset = 0;

    while(idx < t) {
        int nlines = (idx == t - 1) ? lastblc_nlines : regblc_nlines;

        _blckinfo[idx].blck_idx = idx;
        _blckinfo[idx].frst_idx = offset;
        _blckinfo[idx].lst_idx = offset + nlines;
        _blckinfo[idx].is_avbil = true;

        printf("Block %d: lines [%d - %d], length = %d, available = %s\n",
                _blckinfo[idx].blck_idx,
                _blckinfo[idx].frst_idx,
                _blckinfo[idx].lst_idx,
                nlines,
                _blckinfo[idx].is_avbil ? "true" : "false");

        idx++;
        offset += nlines;
    }
}   


void sigalarmhndl(int sig) {
    timed_out = 1;
}

void siginthndl(int sig) {

    if (mem_file) {
        munmap(mem_file, st.st_size);
        printf("[Manager] Reports: unmapped memory.\n");
    }

    if (arr_infot) {
        free(arr_infot);
        printf("[Manager] Reports: freed task array.\n");
    }

    if (id >= 0) {
        msgctl(id, IPC_RMID, NULL);
        printf("[Manager] Reports: removed message queue.\n");
    }

    exit(0);
}

void set_sigint() {
    struct sigaction sigint;
    sigint.sa_handler = &siginthndl;
    sigint.sa_flags = 0;
    sigemptyset(&sigint.sa_mask); 
    sigaction(SIGINT, &sigint, NULL);
}

int main(int argc, char* argv[]) {
    const char *optstring = "t:f:p:";
    int t_flag = 0, f_flag = 0, p_flag = 0, ret, t = 0; 
    char *f = NULL, *passwd = NULL;

    while ((ret = getopt(argc, argv, optstring)) != -1) {
        switch (ret) {
            case 't':t_flag = 1; t = atoi(optarg); break;
            case 'f':f_flag = 1; f = optarg; break;
            case 'p':p_flag = 1; passwd = optarg; break;
            case '?': return -1;
            default: abort();
        }
    }

    if (f_flag == 0 || p_flag == 0) {
        fprintf(stderr, "Error: no argument -f : <%s> or / and -p : <%s>.\n", f, passwd);
        return ERROR_NOARG;
    }

    printf("----------------------------------------------------\n");
    printf("Source File:           [%s]\n", f);
    printf("Number of tasks:       [%d]\n", t);
    printf("Hash:                  [%s]\n", passwd);
    printf("----------------------------------------------------\n");

    set_sigint();

    mem_file = _ldfltomem(f, &st);

    if (!mem_file){
        fprintf(stderr, "Error: _ldfltomem returned NULL!\n");
        return -1;
    }

    int nlines = 0, regbl_nlines = 0, lastbl_nlines = 0;
    prpr_file(mem_file, st.st_size, t, &nlines, &regbl_nlines, &lastbl_nlines);

    printf("----------------------------------------------------\n");
    printf("Number of lines: %d\n", nlines);
    printf("Number of reg lines: %d\n", regbl_nlines);
    printf("Number of last lines: %d\n", lastbl_nlines);
    printf("----------------------------------------------------\n");
    key_t key = ftok("passwords.txt", 'z');
    id = msgget(key, IPC_CREAT | 0600);


    printf("Key:    %d\n", key);
    printf("id:    %d\n", id);

    struct sigaction sigalarm;
    sigalarm.sa_handler = &sigalarmhndl;
    sigalarm.sa_flags = 0;
    sigemptyset(&sigalarm.sa_mask); 
    sigaction(SIGALRM, &sigalarm, NULL);

    arr_infot = malloc(sizeof * arr_infot * t);

    queue_create(arr_infot, t, regbl_nlines, lastbl_nlines);

    hllmsg _hllmsg;
    errmsg _errmsg;
    int r, s1 = 0, wtasks = 0, s2 = 0, tcnt = 0;
    int e;
    while(t > 0) {
        r = msgrcv(id, &_hllmsg, sizeof(_hllmsg) - sizeof(long), 4, IPC_NOWAIT); // IPC_NOWAIT

        if (r > 0) {
            printf("Worker with PID %d reports!\n", _hllmsg.id);
            if (_hllmsg.ready) {
                wtasks = _hllmsg.tasks - 1;

                while (s1 < t && tcnt < wtasks) {
                    s2++;
                    tcnt++;
                }
                printf("How many tasks: %d (id: %d).\n", _hllmsg.tasks, _hllmsg.id);
                printf("Index 1: %d\n", s1);
                printf("Index 2: %d\n", s2);

                strdt data = {.type = 1, .frt_idx = arr_infot[s1].frst_idx, .scd_idx = arr_infot[s2].lst_idx};
                strcpy(data.file_name, f);
                
                msgsnd(id, &data, sizeof(data) - sizeof(long), 0);
                timed_out = 0;

                alarm(10); 

                bckmsg _bckmsg;
                r = msgrcv(id, &_bckmsg, sizeof(_bckmsg) - sizeof(long), 2, 0);


                // if (r >= 0 && !timed_out) {
                //     printf("Received message from worker.\n");
                //     t -= _hllmsg.tasks;
                // } else if (timed_out) {
                //     printf("Timeout occurred.\n");
                // } 
                printf("r = %d\n", r);
                if (r >= 0) {
                    int essa = alarm(0); 

                    printf("Time: %d\n", essa);
                    if (!timed_out) {
                        printf("Received message from worker.\n");
                    } else {
                        printf("Message received, but after timeout (race condition).\n");
                    }
                }

                s2++;
                s1 = s2;
                tcnt = 0;

                printf("\n\n");
                _hllmsg.ready = false;
            }
        }

        e = msgrcv(id, &_errmsg, sizeof(_errmsg) - sizeof(long), 10, IPC_NOWAIT);
        if (e > 0) {
            printf("[Manager] Reports: worker %d (PID) stopped working.\n", _errmsg.pid);
            printf("[Manager] Reports: new value of \"s1\" is %d.\n", _errmsg.nwline);
        }
    }

    printf("nigga\n");

    msgctl(id, IPC_RMID, NULL);

    munmap(mem_file, st.st_size);
    if (arr_infot)
        free(arr_infot);
    return 0;
}