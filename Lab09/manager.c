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
static int *arr_fails = NULL;
static int id = -1; // normal id
static int pgid= -1; // for progress

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
        _blckinfo[idx].pid = -1;

        printf("Block %d: lines [%d - %d], length = %d \n",
                _blckinfo[idx].blck_idx,
                _blckinfo[idx].frst_idx,
                _blckinfo[idx].lst_idx,
                nlines);

        idx++;
        offset += nlines;
    }
}  

void assign_worker(blckinfo *_blckinfo, pid_t pid, int s1, int s2) {
    while (s1 <= s2) {
        _blckinfo[s1].pid = pid;
        s1++;
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

    if(arr_fails) {
        free(arr_fails);
        printf("[Manager] Reports: freed fail - task array.\n");
    }

    if (id >= 0) {
        msgctl(id, IPC_RMID, NULL);
        printf("[Manager] Reports: removed message queue.\n");
    }

    if (pgid >= 0) {
        msgctl(pgid, IPC_RMID, NULL);
        printf("[Manager] Reports: removed f - message queue.\n");
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
    char *f = NULL, *hash = NULL;

    while ((ret = getopt(argc, argv, optstring)) != -1) {
        switch (ret) {
            case 't':t_flag = 1; t = atoi(optarg); break;
            case 'f':f_flag = 1; f = optarg; break;
            case 'p':p_flag = 1; hash = optarg; break;
            case '?': return -1;
            default: abort();
        }
    }

    if (f_flag == 0 || p_flag == 0) {
        fprintf(stderr, "Error: no argument -f : <%s> or / and -p : <%s>.\n", f, hash);
        return ERROR_NOARG;
    }

    printf("----------------------------------------------------\n");
    printf("Source File:           [%s]\n", f);
    printf("Number of tasks:       [%d]\n", t);
    printf("Hash:                  [%s]\n", hash);
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

    key_t fkey = ftok(".", 'z');
    pgid= msgget(fkey, IPC_CREAT | 0600);


    printf("Key:    %d\n", key);
    printf("FKey:   %d\n", fkey);
    printf("id:    %d\n", id);

    struct sigaction sigalarm;
    sigalarm.sa_handler = &sigalarmhndl;
    sigalarm.sa_flags = 0;
    sigemptyset(&sigalarm.sa_mask); 
    sigaction(SIGALRM, &sigalarm, NULL);

    arr_infot = malloc(sizeof * arr_infot * t);
    arr_fails = malloc(sizeof * arr_fails * t);
    for (int i = 0; i < t; i++) arr_fails[i] = 0;

    queue_create(arr_infot, t, regbl_nlines, lastbl_nlines);

    hllmsg _hllmsg;
    errmsg _errmsg;
    int r, s1 = 0, wtasks = 0, s2 = 0, tcnt = 0, at = t;
    bool is_fail = false, is_checking = false;

    while(at > 0) {
        r = msgrcv(id, &_hllmsg, sizeof(_hllmsg) - sizeof(long), 4, IPC_NOWAIT); // IPC_NOWAIT

        if (r > 0) {
            printf("[Manager] Worker with PID %d reports!\n", _hllmsg.id);
            printf("A) s1 = %d\n", s1);
            printf("Is fail? %d\n", is_fail);
            wtasks = _hllmsg.tasks - 1;

            if (is_fail && is_checking) {
                int temp = 0;
                printf("\n[Manager] Reports: re-run fails.\n");

                for (int i = 0; i < t; i++) {
                    if (arr_fails[i] == 1) {
                        arr_fails[i] = 0;
                        temp++;
                        if (temp == 1) s1 = i;

                        if (i + 1 == t || arr_fails[i + 1] == 0) break;
                    }
                }

                s2 = s1;
            }


            while (s1 < t && tcnt < wtasks) {
                s2++;
                tcnt++;
            }

            printf("| How many tasks: %d (id: %d).\n", _hllmsg.tasks, _hllmsg.id);
            printf("| Index 1: %d\n", s1);
            printf("| Index 2: %d\n", s2);

            strdt data = {.type = 1, .frt_idx = arr_infot[s1].frst_idx, .scd_idx = arr_infot[s2].lst_idx};
            strcpy(data.file_name, f);
            strcpy(data.hash, hash);
            
            msgsnd(id, &data, sizeof(data) - sizeof(long), 0);

            assign_worker(arr_infot, _hllmsg.id, s1, s2);

            for (int j = s1; j <= s2; j++) { // temp
                printf("\n\tPid: %d", arr_infot[j].pid);
            }
            printf("\n");
            timed_out = 0;

            alarm(10); 

            bckmsg _bckmsg;
            r = msgrcv(id, &_bckmsg, sizeof(_bckmsg) - sizeof(long), TYPE_BCKMSG_QUE, 0);

            if (r >= 0) {
                alarm(0); 

                if (!timed_out) {
                    printf("Received message from worker.\n");
                } else {
                    printf("Message received, but after timeout (race condition).\n");
                }
            }

            s2++;
            s1 = s2;
            tcnt = 0;
            printf("B) s1 = %d\n", s1);
            printf("\n\n");

            if (s1 == t) is_checking = true;
        }

        r = msgrcv(id, &_errmsg, sizeof(_errmsg) - sizeof(long), 10, IPC_NOWAIT);
        if (r > 0) {
            printf("[Manager] Reports: worker %d (PID) stopped working.\n", _errmsg.pid);
            printf("[Manager] Reports: new value of \"s1\" is %d.\n", _errmsg.nwline);

            for (int i = 0; i < t; i++) {
                if (arr_infot[i].pid == _errmsg.pid) arr_fails[i] = 1;
            }

            for (int i = 0; i < t; i++) { // temp
                if (arr_infot[i].pid == _errmsg.pid) printf("\n\tError in index: %d", i);
            }

            printf("\ts1 = %d\n", s1);
            is_fail = true;
            printf("\n\n");
        }

        bckmsg _bckmsg;
        r = msgrcv(id,  &_bckmsg, sizeof(_bckmsg) - sizeof(long), TYPE_FNDEND_BCKMSG_QUE, IPC_NOWAIT);
        if (r > 0) {
            printf("[Manager] Reports: result from worker %d is %d.\n", _bckmsg.pid, _bckmsg.found);
            int mtasks = _bckmsg.mtasks;
            at -= mtasks;

            if (_bckmsg.found) 
                break;
            
            printf("[Manager] Reports: made tasks: %d / %d (worker / all). \n", mtasks, t);
            //printf("\n");
        }
    }

    printf("nigga\n");

    msgctl(id, IPC_RMID, NULL);
    msgctl(pgid, IPC_RMID, NULL);

    munmap(mem_file, st.st_size);
    if (arr_infot)
        free(arr_infot);

    if (arr_fails)
        free(arr_fails);
    return 0;
}