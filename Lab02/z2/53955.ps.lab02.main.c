// PS IS1 320 LAB02
// Bartłomiej Szewczyk
// sb53955@zut.edu.pl
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <utmpx.h> // for getutxent()
#include <pwd.h> // for getpwnam()
#include <unistd.h>
#include "lib.h"
#define USER_PROCCESS 7

int main() {
    struct utmpx * _utmpx;
    struct passwd * _passwd;

    int userSeqID = 0; 
    while (_utmpx = getutxent()) {
        if (_utmpx->ut_type == USER_PROCCESS) {
            userSeqID++;
            _passwd = getpwnam(_utmpx->ut_user);
            showUser(_utmpx, _passwd, userSeqID);
        }
    }
    endutxent();
    return 0;
}