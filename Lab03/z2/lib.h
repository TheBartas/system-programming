#ifndef _LAB03
#define _LAB03

void get_user_host_flag(struct utmpx * _utmpx, int userSeqID);
void get_user_group_flag(struct utmpx * _utmpx, int userSeqID);
void get_user_extended_data(struct utmpx * _utmpx, int userSeqID);
char * concat_groups(gid_t *groups, int ngroups, int size);
char * get_groups(struct passwd * _passwd);


#endif