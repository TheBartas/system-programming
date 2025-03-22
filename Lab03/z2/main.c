#include <stdio.h>
#include <pwd.h>
#include <utmpx.h>
#include <unistd.h> // for getopt
#include <stdlib.h>
#include <dlfcn.h>

char * (*get_groups)(struct passwd *);


void get_user_host_flag(struct utmpx * _utmpx, int userSeqID) {
    while (_utmpx = getutxent()) {
        if (_utmpx->ut_type == USER_PROCESS) {
            userSeqID++;
            printf("User No. %d | Login: %s | Host: %s\n", userSeqID, _utmpx->ut_user, _utmpx->ut_host);
        }
    }
}

void get_user_group_flag(struct utmpx * _utmpx, int userSeqID, void *handle) {
    struct passwd * _passwd;
    while (_utmpx = getutxent()) {
        if (_utmpx->ut_type == USER_PROCESS) {
            userSeqID++;
            const char * user_name = _utmpx->ut_user;
            _passwd = getpwnam(user_name);
            get_groups = dlsym(handle, "get_groups");
            char * groups = get_groups(_passwd);
            printf("User No. %d | Login: %s | Groups: [%s]\n", userSeqID, user_name, groups);

            free(groups);
        }
    }
}

void get_user_extended_data(struct utmpx * _utmpx, int userSeqID, void *handle) {
    struct passwd * _passwd;
    while (_utmpx = getutxent()) {
        if (_utmpx->ut_type == USER_PROCESS) {
            userSeqID++;
            const char * user_name = _utmpx->ut_user;
            _passwd = getpwnam(user_name);
            get_groups = dlsym(handle, "get_groups");
            char * groups = get_groups(_passwd);
            printf("User No. %d | Login: %s | Host: %s | Groups: [%s]\n", userSeqID, user_name, _utmpx->ut_host, groups);

            free(groups);
        }
    }

}

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
    
    if (hFlag == 0 && gFlag == 0) {
        printf("Chosen option: \"no flags\"\n");
        get_user_no_flags(_utmpx, userSeqID);

    } else if (hFlag == 1 && gFlag == 0) {
        printf("Chosen option: \"-h flag\"\n");
        get_user_host_flag(_utmpx, userSeqID);

    } else if (hFlag == 0 && gFlag == 1) {
        printf("Chosen option: \"-g flag\"\n");
        get_user_group_flag(_utmpx, userSeqID, handle);
    } else {
        printf("Chosen option: \"-g and -h flag\"\n");
        get_user_extended_data(_utmpx, userSeqID, handle);
    }

    dlclose(handle);
    return 0;
}