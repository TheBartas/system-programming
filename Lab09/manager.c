#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define ERROR_NOARG -2


char *_extmmap(char *file_name, struct stat *st) {
    int fd = open(file_name, O_RDONLY, S_IRUSR);
    if (fstat(fd, st) == -1) {
        fprintf(stderr, "Error: cannot get file size. File <name> [%s]\n", file_name);
        return NULL;
    }

    char *mem_file = NULL;
    if ((mem_file = mmap(NULL, st->st_size, PROT_READ, MAP_SHARED, fd, 0)) == NULL) {
        fprintf(stderr, "Error: mmap file <name> [%s]\n", file_name);
        return NULL;
    }
    return mem_file;
}

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
    char *mem_file = _extmmap(f, &st);
    if (!mem_file){
        fprintf(stderr, "Error: _extmmap returned NULL!\n");
        return -1;
    }

    long int bytfilsz = st.st_size;
    printf("[Info] Size of file (bytes): %ld\n", bytfilsz);



    return 0;
}