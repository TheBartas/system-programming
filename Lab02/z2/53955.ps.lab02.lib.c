// PS IS1 320 LAB02
// Bartłomiej Szewczyk
// sb53955@zut.edu.pl
#include <utmpx.h>
#include <pwd.h>
#include <stdio.h>
#include "lib.h"

void showUser(struct utmpx * _utmpx, struct passwd * _passwd, int userSeqID) {
    printf("User %d | login: %s | user ID: %d | host: %s | device (terminal): %s\n",
        userSeqID, _utmpx->ut_user, _passwd->pw_uid, _utmpx->ut_host, _utmpx->ut_line);
}
