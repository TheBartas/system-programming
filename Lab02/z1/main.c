#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <utmpx.h> // for getutxent()
#include <pwd.h> // for getpwnam()
#include <unistd.h>


int main() {
    struct utmpx * _utmpx;
    struct passwd * _passwd;

    int userSeqID = 0; 
    while (_utmpx = getutxent()) {
        if (_utmpx->ut_type == 7) { // 7 is for USER_PROCCESS
            userSeqID++;
            char * userLogin = _utmpx->ut_user;
            _passwd = getpwnam(userLogin);
            printf("User %d | login: %s | user ID: %d | host: %s | device (terminal): %s\n",
                userSeqID, userLogin, _passwd->pw_uid, _utmpx->ut_host, _utmpx->ut_line);
        }
    }
    endutxent();
    return 0;
}