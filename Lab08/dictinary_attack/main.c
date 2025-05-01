#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <crypt.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#define ERROR_NOARG -2
#define ERROR_FPROC -3
#define ERROR_SIZPROC -4

#define SIZE_TD 1000
#define SIZE_TTD 10000
#define SIZE_OHTD 100000


// 181
// 343
// 516

const char *rl_res = "$6$5MfvmFOaDU$CVt7jU9wJRYz3K98EklAJqp8RMG5NvReUSVK7ctVvc2VOnYVrvyTfXaIgHn2xQS78foEJZBq2oCIqwfdNp.2V1";

pthread_mutex_t isfound_mutex=PTHREAD_MUTEX_INITIALIZER;
bool is_found = false;
int current_proc_size = 0;


int *mem_blocks(char *mem_file, size_t size, int t) {
    int last_block = size % t;
    printf("Last block:         %d [ size ]\n", last_block);
    int regular_block = (size - last_block) / t;
    printf("Regular block:      %d [ size ]\n", regular_block);

    printf("\n\n");

    int *pth_arr = NULL;
    pth_arr = malloc(sizeof * pth_arr * (t + 1));

    pth_arr[0] = 0;
    int k = 1;
    for (int i = regular_block - 1; i < size && k < t + 1; i += regular_block - 1) {
        if (mem_file[i] == '\r') { 
            pth_arr[k++] = i; 
            continue; 
        }
        int a = i;
        while (a < size && mem_file[a] != '\r') a++;
        pth_arr[k++] = a;
    }
    return pth_arr;
}

char *_mmap_rd(char *file_name, struct stat *st) {
    int fd = open(file_name, O_RDONLY, S_IRUSR);
    if (fstat(fd, st) == -1) {
        fprintf(stderr, "Error: cannot get file size. File <name> [%s]\n", file_name);
        return NULL;
    }

    char *mem_file = NULL;
    if ((mem_file = mmap(NULL, st->st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == NULL) {
        fprintf(stderr, "Error: mmap file <name> [%s]\n", file_name);
        return NULL;
    }
    return mem_file;
}


void *proc_block(void *args) {
    FILE *in = (FILE *)args;

    pthread_t tid = pthread_self();
    char * buf = NULL;
    size_t size_buf = 0;

    char *encrpt = NULL; 
    struct crypt_data _crypt_data;

    while (getline(&buf, &size_buf, in) != -1) {
        int buflen = strlen(buf);
        pthread_mutex_lock(&isfound_mutex);
        bool found = is_found;
        current_proc_size += buflen;
        pthread_mutex_unlock(&isfound_mutex);

        if (found) break;

        int idx = buflen - 2;
        if (buf[idx] == '\r') buf[idx] = '\0';

        if ((encrpt = crypt_r(buf, "$6$5MfvmFOaDU$", &_crypt_data)) != NULL) {
            if (strcmp(encrpt, rl_res) == 0) {
                printf("Password found! Result is \"%s\"! [%ld]\n", buf, tid);
                pthread_mutex_lock(&isfound_mutex);
                is_found = true;
                pthread_mutex_unlock(&isfound_mutex);
                break;
            }
        }
        sleep(1);
    }
    free(buf);
    fclose(in);
    pthread_exit(0);
    return NULL;
}

int main(int argc, char *argv[]) {
    const char *optstring = "t:f:p:";
    int t_flag = 0, f_flag = 0, p_flag = 0;
    int ret, t = 0; 
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

    printf("Threads:                          [%d] (if zero then time benchmark)\n", t);
    printf("Source File:                      [%s]\n", f);
    printf("Password:                         [%s]\n", passwd);

    if (t_flag == 1) {
        int av_proc;
        if ((av_proc = sysconf(_SC_NPROCESSORS_ONLN)) == -1) {
            fprintf(stderr, "Fail with getting [ _SC_NPROCESSORS_ONLN ] : [ %d ] data.\n", av_proc);
            return ERROR_FPROC;
        }

        printf("Available processes (max thread): [%d]\n", av_proc);

        if (av_proc < t) {
            fprintf(stderr, "Error: flag argument -t <int> [%d] should be less or equels to <av_proc> [%d].\n", t, av_proc);
            return ERROR_SIZPROC;
        }

        struct stat st;
        char *mem_file = _mmap_rd(f, &st);
        printf("\nSize: %ld\n", st.st_size);


        int *pth_arr = mem_blocks(mem_file, st.st_size, t);

        pthread_t *threads = NULL;
        threads = malloc(sizeof * threads * t);

        for (int i = 0; i < t; i++) {
            size_t offset = pth_arr[i];
            size_t leng = pth_arr[i+1] - offset;
            FILE *in = fmemopen(&mem_file[offset], leng, "r");
            pthread_create(&threads[i], NULL, proc_block, in);
        }

        while (true) {
            pthread_mutex_lock(&isfound_mutex);
            bool done = is_found || current_proc_size >= st.st_size;
            pthread_mutex_unlock(&isfound_mutex);
        
            if (done) break;
        
            printf("Progress: %d / %ld\r", current_proc_size, st.st_size);
            fflush(stdout);
        }

        for (int i = 0; i < t; i++) {
            if (pthread_join(threads[i], NULL) != 0) {
                printf("Error joining thread\n");
                return 1;
            }
        }


        free(threads);
        free(pth_arr);

        if (!is_found) printf("Password not found!\n");
        return 0;
    }

    return 0;
}