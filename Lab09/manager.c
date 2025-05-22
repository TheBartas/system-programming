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

void create_queue(blckinfo *_blckinfo, int t, int regblc_nlines, int lastblc_nlines) {
    int idx = 0;
    int offset = 0;

    while(idx < t) {
        int nlines = (idx == t - 1) ? lastblc_nlines : regblc_nlines;

        _blckinfo[idx].blck_idx = idx;
        _blckinfo[idx].frst_idx = offset;
        _blckinfo[idx].lst_idx = offset + nlines - 1;
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

    struct stat st;
    char *mem_file = _ldfltomem(f, &st);

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
    int id = msgget(key, IPC_CREAT | 0600);

    printf("Key:    %d\n", key);
    printf("id:    %d\n", id);

    struct sigaction sigalarm;
    sigalarm.sa_handler = &sigalarmhndl;
    sigalarm.sa_flags = 0;
    sigemptyset(&sigalarm.sa_mask); 
    sigaction(SIGALRM, &sigalarm, NULL);

    blckinfo *arr_infot = NULL;
    arr_infot = malloc(sizeof * arr_infot * t);

    create_queue(arr_infot, t, regbl_nlines, lastbl_nlines);

    hllmsg _hllmsg;
    int r, i = 0;
    while(t > 0) {
        r = msgrcv(id, &_hllmsg, sizeof(_hllmsg) - sizeof(long), 4, 0); // IPC_NOWAIT
        if (r < 0) {
            printf("nigga won't work!\n");
            continue;
        }
        printf("Worker with PID %d reports!\n", _hllmsg.id);
        if (_hllmsg.ready) {
            strdt data = {.type = 1, .offset = i * 10, .length = 10};
            strcpy(data.file_name, f);
            
            msgsnd(id, &data, sizeof(data) - sizeof(long), 0);

            alarm(4); 

            bckmsg _bckmsg;
            int r = msgrcv(id, &_bckmsg, sizeof(_bckmsg) - sizeof(long), 0, 0);

            if (r >= 0 && !timed_out) {
                printf("Received message from worker.\n");
            } else if (timed_out) {
                printf("Timeout occurred.\n");
                //break;
            } else {
                perror("msgrcv failed");
                break;
            }
            t--;
            i++;
            _hllmsg.ready = false;
        }
    }

    printf("nigga\n");

    msgctl(id, IPC_RMID, NULL);

    munmap(mem_file, st.st_size);
    if (arr_infot)
        free(arr_infot);
    return 0;
}