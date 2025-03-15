#include <utmpx.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include "lib.h"

void get_user_host_flag(struct utmpx * _utmpx, int userSeqID) {
    while (_utmpx = getutxent()) {
        if (_utmpx->ut_type == USER_PROCESS) {
            userSeqID++;
            printf("User No. %d | Login: %s | Host: %s\n", userSeqID, _utmpx->ut_user, _utmpx->ut_host);
        }
    }
}

void get_user_group_flag(struct utmpx * _utmpx, int userSeqID) {
    struct passwd * _passwd;
    while (_utmpx = getutxent()) {
        if (_utmpx->ut_type == USER_PROCESS) {
            userSeqID++;
            const char * user_name = _utmpx->ut_user;
            _passwd = getpwnam(user_name);
            char * groups = get_groups(_passwd);
            printf("User No. %d | Login: %s | Groups: [%s]\n", userSeqID, user_name, groups);

            free(groups);
        }
    }
}

void get_user_extended_data(struct utmpx * _utmpx, int userSeqID) {
    struct passwd * _passwd;
    while (_utmpx = getutxent()) {
        if (_utmpx->ut_type == USER_PROCESS) {
            userSeqID++;
            const char * user_name = _utmpx->ut_user;
            _passwd = getpwnam(user_name);
            char * groups = get_groups(_passwd);
            printf("User No. %d | Login: %s | Host: %s | Groups: [%s]\n", userSeqID, user_name, _utmpx->ut_host, groups);

            free(groups);
        }
    }

}

char * concat_groups(gid_t *groups, int ngroups, int size) {
    struct group * _group;
    char * result_group = NULL;
    result_group = malloc(sizeof * result_group * size);
    result_group[0] = '\0';
    for (int i = 0; i < ngroups; i++) {
        _group = getgrgid(groups[i]);

        if (_group != NULL) {
            char * gr_name = _group->gr_name;
            strcat(result_group, gr_name); 
            if (ngroups - 1 != i) 
                strcat(result_group, ", ");

        }
    }
    return result_group;
}

char * get_groups(struct passwd * _passwd) {
    struct group * _group;
    int ngroups = 0;
    gid_t *groups; // an integer data type used to represent group IDs

    getgrouplist(_passwd->pw_name, _passwd->pw_gid, NULL, &ngroups);

    groups = malloc(sizeof * groups * ngroups);

    getgrouplist(_passwd->pw_name, _passwd->pw_gid, groups, &ngroups);

    int size = 0;

    for (int i = 0; i < ngroups; i++) {
        _group = getgrgid(groups[i]);

        if (_group != NULL) {
            const char * gr_name = _group->gr_name;
            if (ngroups - 1 == i) 
                size+=strlen(gr_name) + 1;
            else 
                size+=strlen(gr_name) + 2; // because of ", "
        }
    }
 
    char * result = concat_groups(groups, ngroups, size);

    free(groups);  
    return result; 
}