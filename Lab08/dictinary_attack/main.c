#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <crypt.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#define ERROR_NOARG -2
#define ERROR_FPROC -3
#define ERROR_SIZPROC -4

#define SIZE_TD 1000
#define SIZE_FTD 5000
#define SIZE_TTD 10000
#define SIZE_OHTD 100000


//const char *rl_res = "$6$5MfvmFOaDU$CVt7jU9wJRYz3K98EklAJqp8RMG5NvReUSVK7ctVvc2VOnYVrvyTfXaIgHn2xQS78foEJZBq2oCIqwfdNp.2V1";

pthread_mutex_t isfound_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t progress_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t set_isfound_mutex = PTHREAD_MUTEX_INITIALIZER;
bool is_found = false;
long int current_proc_size = 0;

typedef struct {
    FILE *in;
    const char *rle_res;
} thread_args_t;

typedef struct {
    FILE *in;
    int lines;
} benchmark_args_t;

int calc_nlines(char *mem_file, size_t size) { 
    FILE *in = fmemopen(mem_file, size, "r");
    char *buf = NULL;
    size_t size_buf = 0;
    int nlines = 0;
    
    while (getline(&buf, &size_buf, in) != -1) {
        nlines++;
    }

    free(buf);
    fclose(in);
    return nlines;
}

int* calc_nlines_sizes(int size, int t) {  
    int last_block = size % t;
    int regular_block = (size - last_block) / t;

    int *arr = NULL;
    arr = malloc(sizeof * arr * 2);

    arr[0] = regular_block;
    arr[1] = last_block + regular_block;
    return arr;
}

void *proc_progress_bar(void *args) {
    int * total_size = (int*)args;
    printf("Size: %d\n", *total_size);
    while (true) {
        pthread_mutex_lock(&progress_mutex);
        double progress = (double)current_proc_size / (double)(*total_size) * 100;
        bool done = (is_found || current_proc_size >= *total_size);
        pthread_mutex_unlock(&progress_mutex);
    
        printf("Progress: %.02f %%.\r", progress);
        fflush(stdout);
        if (done) break;
    }
    return NULL;
}

int *mem_blocks(char *mem_file, size_t size, int t) {
    int last_block = size % t;
    int regular_block = (size - last_block) / t;

    int *pth_arr = NULL;
    pth_arr = malloc(sizeof * pth_arr * (t + 1));

    pth_arr[0] = 0;
    int k = 1;
    for (int i = regular_block - 1; i < size && k < t + 1; i += regular_block - 1) {
        if (mem_file[i] == '\n') { 
            pth_arr[k++] = i; 
            continue; 
        }
        int a = i;
        while (a < size && mem_file[a] != '\n') a++;
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
    thread_args_t *data = (thread_args_t *)args;
    FILE *in = data->in;
    const char *rl_res = data->rle_res;

    pthread_t tid = pthread_self();
    char * buf = NULL;
    size_t size_buf = 0;

    char *encrpt = NULL; 
    struct crypt_data _crypt_data;
    while (getline(&buf, &size_buf, in) != -1) {
        int buflen = strlen(buf);

        pthread_mutex_lock(&progress_mutex);
        bool found = is_found;
        current_proc_size++;
        pthread_mutex_unlock(&progress_mutex);

        if (found) break;

        int idx = buflen - 1; // dla pliku example.txt -2
        if (buf[idx] == '\n') buf[idx] = '\0';

        if ((encrpt = crypt_r(buf, "$6$5MfvmFOaDU$", &_crypt_data)) != NULL) {
            if (strcmp(encrpt, rl_res) == 0) {
                printf("Password found! Result is \"%s\"! [%ld]\n", buf, tid);
                pthread_mutex_lock(&set_isfound_mutex);
                is_found = true;
                pthread_mutex_unlock(&set_isfound_mutex);
                break;
            }
        }
    }
    free(buf);
    fclose(in);
    pthread_exit(0);
    return NULL;
}

void *proc_benchmark_block(void *args) {
    benchmark_args_t *data = (benchmark_args_t *)args;
    FILE *in = data->in;
    int nlines = data->lines;

    pthread_t tid = pthread_self();
    char * buf = NULL;
    size_t size_buf = 0;

    char *encrpt = NULL; 
    struct crypt_data _crypt_data;
    int k = 0;
    while (getline(&buf, &size_buf, in) != -1) {
        if (k++ >= nlines) break;

        int buflen = strlen(buf);
        pthread_mutex_lock(&progress_mutex);
        current_proc_size++;
        pthread_mutex_unlock(&progress_mutex);

        int idx = buflen - 1;
        if (buf[idx] == '\n') buf[idx] = '\0';

        if ((encrpt = crypt_r(buf, "$6$5MfvmFOaDU$", &_crypt_data)) == NULL) {
            fprintf(stderr, "Error: encryption fail crypt_r [%s]", encrpt);
            return NULL;;
        }
    }
    free(buf);
    fclose(in);
    pthread_exit(0);
    return NULL;
}

void benchmark(int av_proc, char *f) {
    struct stat st;
    char *mem_file = _mmap_rd(f, &st);
    printf("Benchmark start...\n");
    av_proc = 4;
    for (int i = 1; i <= av_proc; i++) {
        int *pth_arr = mem_blocks(mem_file, st.st_size, i);

        pthread_t *threads = NULL;
        threads = malloc(sizeof * threads * i);
        benchmark_args_t *arg = NULL;
        arg = malloc(sizeof * arg * i);


        int size = SIZE_FTD;
        int nlines = calc_nlines(mem_file, i);
        int *lines_arr = calc_nlines_sizes(size, i);

        pthread_t monitthrd;
        pthread_create(&monitthrd, NULL, proc_progress_bar, &size);

        struct timespec start, stop;
        clock_gettime(CLOCK_MONOTONIC, &start);
    
        for (int j = 0; j < i; j++) {
            size_t offset = pth_arr[j];
            size_t leng = pth_arr[j+1] - offset;
            FILE *in = fmemopen(&mem_file[offset], leng, "r");
            arg[j].in = in;
            arg[j].lines = (j != i - 1) ? lines_arr[0] : lines_arr[1];
            
            pthread_create(&threads[j], NULL, proc_benchmark_block, &arg[j]);
        }

        for (int j = 0; j < i; j++) {
            pthread_join(threads[j], NULL);
        }

        clock_gettime(CLOCK_MONOTONIC, &stop); 
        double diff = (stop.tv_sec - start.tv_sec) * 1000.0 + (stop.tv_nsec - start.tv_nsec) / 1000000.0;
        printf("Using [%d] threads ended in: %f [ms].\n", i, diff);

        pthread_join(monitthrd, NULL);

        pthread_mutex_lock(&progress_mutex);
        current_proc_size = 0;
        pthread_mutex_unlock(&progress_mutex);

        free(arg);
        free(threads);
        free(pth_arr);
        free(lines_arr);
    }
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

    int av_proc;
    if ((av_proc = sysconf(_SC_NPROCESSORS_ONLN)) == -1) {
        fprintf(stderr, "Fail with getting [ _SC_NPROCESSORS_ONLN ] : [ %d ] data.\n", av_proc);
        return ERROR_FPROC;
    }

    if (t_flag == 1) {
        printf("Available processes (max thread): [%d]\n", av_proc);

        if (av_proc < t) {
            fprintf(stderr, "Error: flag argument -t <int> [%d] should be less or equels to <av_proc> [%d].\n", t, av_proc);
            return ERROR_SIZPROC;
        }

        struct stat st;
        char *mem_file = _mmap_rd(f, &st);
        printf("\nSize: %ld\n", st.st_size);


        int *pth_arr = mem_blocks(mem_file, st.st_size, t);
        int nlines = calc_nlines(mem_file, st.st_size);

        pthread_t monitthrd;
        pthread_create(&monitthrd, NULL, proc_progress_bar, &nlines);

        pthread_t *threads = NULL;
        threads = malloc(sizeof * threads * t);

        struct timespec start;
        clock_gettime(CLOCK_MONOTONIC, &start);

        for (int i = 0; i < t; i++) { 
            size_t offset = pth_arr[i];
            size_t leng = pth_arr[i+1] - offset;
            FILE *in = fmemopen(&mem_file[offset], leng, "r");

            thread_args_t *arg = malloc(sizeof(thread_args_t));
            arg->in = in;
            arg->rle_res = passwd; 
        
            pthread_create(&threads[i], NULL, proc_block, arg);
        }

        for (int i = 0; i < t; i++) {
            if (pthread_join(threads[i], NULL) != 0) {
                printf("Error joining thread\n");
                return 1;
            }
        }

        struct timespec stop;
        clock_gettime(CLOCK_MONOTONIC, &stop); 
        double diff = (stop.tv_sec - start.tv_sec) * 1000.0 + (stop.tv_nsec - start.tv_nsec) / 1000000.0;
        printf("Using [%d] threads ended in: %f [ms].\n", t, diff);

        pthread_join(monitthrd, NULL);
        free(threads);
        free(pth_arr);

        if (!is_found) printf("Password not found!\n");
        return 0;

    } else {
        benchmark(av_proc, f);
        return 0;
    }
    return -1;
}