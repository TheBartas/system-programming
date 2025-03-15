#include <stdio.h>
#include <pwd.h>
#include <utmpx.h>
#include <unistd.h> // for getopt
#include <stdlib.h>
#include <dlfcn.h>


void (*get_user_host_flag)(struct utmpx *, int);
void (*get_user_group_flag)(struct utmpx *, int);
void (*get_user_extended_data)(struct utmpx *, int);

void get_user_no_flags(struct utmpx * _utmpx, int userSeqID) {
    while (_utmpx = getutxent()) {
        if (_utmpx->ut_type == USER_PROCESS) { 
            userSeqID++;
            printf("User No. %d | Login: %s\n", userSeqID, _utmpx->ut_user);
        }
    }
}

int main(int argc, char **argv) {
    void *handle = dlopen("./libgroup.so", RTLD_LAZY); // delete .so to test
    int hFlag = 0, gFlag = 0, opt;
    const char* optstring = "hg";

    if (!handle) {
        printf("Error!\n");
        dlerror();
    } else {
        while ((opt = getopt(argc, argv, optstring)) != -1) {
            switch (opt) {
                case 'h': hFlag = 1; break;
                case 'g': gFlag = 1; break;
                default: abort();
            }
        }
    }

    struct utmpx * _utmpx;
    int userSeqID = 0;

    printf("G = %d\n", gFlag);
    printf("H = %d\n", hFlag);
    
    if (hFlag == 0 && gFlag == 0) {
        printf("Chosen option: \"no flags\"\n");
        get_user_no_flags(_utmpx, userSeqID);

    } else if (hFlag == 1 && gFlag == 0) {
        printf("Chosen option: \"-h flag\"\n");
        get_user_host_flag = dlsym(handle, "get_user_host_flag");
        get_user_host_flag(_utmpx, userSeqID);

    } else if (hFlag == 0 && gFlag == 1) {
        printf("Chosen option: \"-g flag\"\n");
        get_user_group_flag = dlsym(handle, "get_user_group_flag");
        get_user_group_flag(_utmpx, userSeqID);
    } else {
        printf("Chosen option: \"-g and -h flag\"\n");
        get_user_extended_data = dlsym(handle, "get_user_extended_data");
        get_user_extended_data(_utmpx, userSeqID);
    }

    dlclose(handle);
    return 0;
}