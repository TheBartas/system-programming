#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#define ERROR_NOARG -2
#define ERROR_FPROC -3
#define ERROR_SIZPROC -4

#define SIZE_TD 1000
#define SIZE_TTD 10000
#define SIZE_OHTD 100000

char *_mmap_rd(char *file_name, struct stat st) {
    int fd = open(file_name, O_RDONLY, S_IRUSR);
    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "Error: cannot get file size. File <name> [%s]\n", file_name);
        return NULL;
    }

    char *mem_file = NULL;
    if ((mem_file = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == NULL) {
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
        char *mem_file = _mmap_rd(f, st);

        for (int i = 0;i< SIZE_TD;i++) {
            if (mem_file[i] == '\r') {
                printf("\n");
                continue;
            }
            printf("%c", mem_file[i]);
        }
        
        printf("%ld", st.st_nlink);


    }



    return 0;
}