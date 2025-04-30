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
#define ERROR_NOARG -2
#define ERROR_FPROC -3
#define ERROR_SIZPROC -4

#define SIZE_TD 1000
#define SIZE_TTD 10000
#define SIZE_OHTD 100000

const char *rl_res = "$6$5MfvmFOaDU$CVt7jU9wJRYz3K98EklAJqp8RMG5NvReUSVK7ctVvc2VOnYVrvyTfXaIgHn2xQS78foEJZBq2oCIqwfdNp.2V1";


int *mem_blocks(char *mem_file, size_t size, int t) {
    int last_block = size % t;
    printf("Last block:         %d [ size ]\n", last_block);
    int regular_block = (size - last_block) / t;
    printf("Regular block:      %d [ size ]\n", regular_block);

    printf("\n\n");

    int *idx_arr = NULL;
    idx_arr = malloc(sizeof * idx_arr * t);

    int k = 0;
    for (int i = regular_block - 1; i < size; i += regular_block - 1) {
        if (mem_file[i] == '\r') { 
            idx_arr[k] = i; 
            k++;
            continue; 
        }
        int a = i;
        while (a < size && mem_file[a] != '\r') a++;
        idx_arr[k] = a;
        k++;
    }
    return idx_arr;
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

        printf("\n");

        int *idx_arr = mem_blocks(mem_file, st.st_size, t);

        char *encrpt = NULL; 
        struct crypt_data _crypt_data;

        char *buf = NULL;
        char *tmbf = NULL;
        size_t sbf = 0;
        //FILE *stream = open_memstream(&buf, &sbf);
        bool is_found = false;


        //fputs(mem_file, stream);
        //fclose(stream);
        //printf("%s", buf);
        buf = malloc(20);
        FILE *in = fmemopen(mem_file, st.st_size, "r");
        // fputs(mem_file, in);
        // fgets(buf, sizeof(buf), in);
        // fclose(in);
        // printf("%s", buf);
        // strcspn();
        
        // while (fgets(buf, sizeof(buf), in)) {
        //     // char *token = strtok(in, ";");
        //     // while (token) {
        //     //     printf("Fragment: %s\n", token);
        //     //     token = strtok(NULL, ";");
        //     // }
        //     //printf("%s", buf);
        //     if (strcmp(buf, "dees\r\n") == 0) {
        //     }
        // }

        char * expbuf = NULL;
        size_t size_expbuf = 0;
        while (getline(&expbuf, &size_expbuf, in) != -1) {
            int len = strlen(expbuf) - 2;
            if (expbuf[len] == '\r') expbuf[len] = '\0';

            if ((encrpt = crypt_r(expbuf, "$6$5MfvmFOaDU$", &_crypt_data)) != NULL) {
                if (strcmp(encrpt, rl_res) == 0) {
                    is_found = true;
                    break;
                }
            }

        }

        /*
        for (int i = 0; i < st.st_size; i++) {
            if (mem_file[i] == '\r' && mem_file[i + 1] == '\n') { // chodzi o to, że hasła w pliku kończą się jako \r\n
                i++;
                fclose(stream);
                if ((encrpt = crypt_r(buf, "$6$5MfvmFOaDU$", &_crypt_data)) != NULL) {
                    if (strcmp(encrpt, rl_res) == 0) {
                        is_found = true;
                        break;
                    }
                }
                free(buf);
                sbf = 0;
                stream = open_memstream(&buf, &sbf);   
                continue;
            }
            fputc(mem_file[i], stream);
        }

        if (!is_found) {
            fclose(stream);
            if ((encrpt = crypt_r(buf, "$6$5MfvmFOaDU$", &_crypt_data)) != NULL) {
                if (strcmp(encrpt, rl_res) == 0) {
                    is_found = true;
                }
            }
        }
        */
        if (is_found) printf("Password found! Result is \"%s\"!\n", expbuf);


        free(buf);
        free(idx_arr);
    }

    return 0;
}