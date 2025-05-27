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
#include <pthread.h>

#include "data.h"

#define ERROR_NOARG -2

pthread_mutex_t mtxnotfnd = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtxisrterr = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtxisfail = PTHREAD_MUTEX_INITIALIZER;

static sig_atomic_t timed_out = 0; // volatile
static char *mem_file = NULL;
static struct stat st;
static blckinfo *arr_infot = NULL;
static int *arr_fails = NULL;
static int id = -1; // normal id
static int pgid = -1; // for progress
static bool isend = false;
static bool is_fail = false;

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
        _blckinfo[idx].isdone = false;

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

void *control_workers(void *args) {
    thread_args_t *data = (thread_args_t *)args;
    int t = data->t;
    printf("[Manager] Reports: control workers. {t = %d}\n", t);

    while(1) {
        if (isend) break;
        for (int i = 0; i < t; i++) {
            pid_t pid;
            if ((pid = arr_infot[i].pid) > 0) {
                int s; 
                if ((s = kill(pid, 0)) == -1) {
                    printf("[Manager / Error] Worker with pid %d has been killed and is not reporting.\n", pid);
                    printf("[Manager / Info] Task [%d] is again available.\n", i);
                    printf("\n");
                    arr_infot[i].pid = -1;

                    pthread_mutex_lock(&mtxisrterr);
                    arr_fails[i] = 1;
                    pthread_mutex_unlock(&mtxisrterr);

                    pthread_mutex_lock(&mtxisfail);
                    is_fail = true;
                    pthread_mutex_unlock(&mtxisfail);
                }; 
            }
        }

        // usleep(0);
    }
}

int smaller(int a, int b) {
    return (a >= b) ? b : a;
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
    printf("id:     %d\n", id);

    struct sigaction sigalarm;
    sigalarm.sa_handler = &sigalarmhndl;
    sigalarm.sa_flags = 0;
    sigemptyset(&sigalarm.sa_mask); 
    sigaction(SIGALRM, &sigalarm, NULL);

    arr_infot = malloc(sizeof * arr_infot * t);
    arr_fails = malloc(sizeof * arr_fails * t);
    for (int i = 0; i < t; i++) arr_fails[i] = 0;

    queue_create(arr_infot, t, regbl_nlines, lastbl_nlines);

    thread_args_t args = {.t = t};
    pthread_t work_cntr_tid; // worker_control_tid
    pthread_create(&work_cntr_tid, NULL, control_workers, &args);

    hllmsg _hllmsg;
    int r, wtasks = 0,s1 = 0, s2 = 0, tcnt = 0, at = t, tmp_num_tsks = t - 1;
    bool is_checking = false;

    while(at > 0) {
        r = msgrcv(id, &_hllmsg, sizeof(_hllmsg) - sizeof(long), TYPE_HLLMSG_QUE, IPC_NOWAIT); // IPC_NOWAIT

        if (r > 0) {
            printf("[Manager] Worker with PID %d reports!\n", _hllmsg.id);
            wtasks = _hllmsg.tasks - 1;

            if (is_fail && is_checking) {
                int temp = 0;
                printf("\n[Manager] Reports: re-run fails.\n");

                for (int i = 0; i < t; i++) {
                    if (arr_fails[i] == 1) {
                        pthread_mutex_lock(&mtxisrterr);
                        arr_fails[i] = 0;
                        pthread_mutex_unlock(&mtxisrterr);
                        temp++;
                        if (temp == 1) s1 = i;

                        if (i + 1 == t || arr_fails[i + 1] == 0) break;
                    }
                }

                s2 = s1;
            }


            printf("[Manager] Number of available tasks: %d\n", tmp_num_tsks + 1);
            int max_tasks = smaller(wtasks, tmp_num_tsks);
            printf("[Manager] Worker do: %d\n", max_tasks + 1);

            while (s1 < t && tcnt < max_tasks) {
                s2++;
                tcnt++;
            }

            tmp_num_tsks -= ++max_tasks; 

            printf("[Manager] Number of available tasks: %d\n", tmp_num_tsks + 1);
            printf("\t|> How many tasks: %d (id: %d).\n", _hllmsg.tasks, _hllmsg.id);
            printf("\t|> Index 1: %d\n", s1);
            printf("\t|> Index 2: %d\n", s2);

            strdt data = {.type = 1, .frt_idx = arr_infot[s1].frst_idx, .scd_idx = arr_infot[s2].lst_idx, .tasks = max_tasks};
            strcpy(data.file_name, f);
            strcpy(data.hash, hash);
            
            msgsnd(id, &data, sizeof(data) - sizeof(long), 0);

            assign_worker(arr_infot, _hllmsg.id, s1, s2);

            printf("\n");
            timed_out = 0;

            alarm(2); 

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
            printf("\n\n");
        }

        bckmsg _bckmsg;
        r = msgrcv(id,  &_bckmsg, sizeof(_bckmsg) - sizeof(long), TYPE_FNDEND_BCKMSG_QUE, IPC_NOWAIT);
        if (r > 0) {
            printf("[Manager] Worker with <PID> %d did his work. Result is %d.\n", _bckmsg.pid, _bckmsg.found);
            int mtasks = _bckmsg.mtasks;
            at -= mtasks;
            
            printf("[Manager] Yet: %d tasks.\n", at);
            if (_bckmsg.found) {
                pthread_mutex_lock(&mtxnotfnd);
                isend = true;
                pthread_mutex_unlock(&mtxnotfnd);
                break;
            }

            pthread_mutex_lock(&mtxisrterr);
            for (int i = 0; i < t; i++) {
                if (arr_infot[i].pid == _bckmsg.pid) arr_infot[i].pid = -2;
            }
            pthread_mutex_unlock(&mtxisrterr);
        }

        if (s1 == t) { 
            is_checking = true;

        }
    }

    printf("Main loop ended!\n");
    pthread_join(work_cntr_tid, NULL);

    msgctl(id, IPC_RMID, NULL);
    msgctl(pgid, IPC_RMID, NULL);

    munmap(mem_file, st.st_size);
    if (arr_infot)
        free(arr_infot);

    if (arr_fails)
        free(arr_fails);
    return 0;
}