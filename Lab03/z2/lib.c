#include <utmpx.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include "lib.h"

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