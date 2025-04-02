#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>

bool loop = true;
volatile sig_atomic_t number_of_process = 0;

void sigchild_handler(int sigNo, siginfo_t *si, void *sc) {
    time_t t;
    struct tm *_tm;
    int status;
    number_of_process--;
    time(&t);
    _tm = localtime(&t);
    printf("\t\t[ %d ] [ %d ] [ %02d / %02d / %04d %02d:%02d:%02d ]\n", si->si_pid, WEXITSTATUS(status),
            _tm->tm_mday, _tm->tm_mon + 1, _tm->tm_year + 1900, _tm->tm_hour, _tm->tm_min, _tm->tm_sec
        );
}

void sigalarm_handler(int sigNo, siginfo_t *si, void *sc) {
    exit(3); 
}

void sigint_handler(int sigNo, siginfo_t *si, void *sc) {
    loop = false;
}

void _sleep(int seconds) {
    struct timespec req, rem;
    req.tv_sec = seconds;
    req.tv_nsec = 0;
    while (nanosleep(&req, &rem) == -1) {
        req = rem;
    }
}

unsigned long long inf_factorial() {
    unsigned long long factorial = 1;
    for (int i = 1;; i++) {
        factorial *= i;
    }
    return factorial;
}

int main(int argc, char* argv[]) {
    int w_flag = 0, m_flag = 0;
    int ret, w, m;
    const char* optstring = "w:m:";

    while((ret = getopt(argc, argv, optstring)) != -1) {
        switch(ret) {
            case 'w': w = atoi(optarg); w_flag = 1; break;
            case 'm': m = atoi(optarg); m_flag = 1; break;
            case '?': return -1;
            default: abort();
        }
    }

    if (w_flag == 0 || m_flag == 0) {
        fprintf(stderr, "[Error]\nTwo flags are needed!\n"); 
        return -1;
    }

    struct sigaction sa_child;
    sigemptyset(&sa_child.sa_mask);
    sa_child.sa_sigaction = sigchild_handler;
    sa_child.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGCHLD, &sa_child, NULL);

    struct sigaction sa_sigint;
    sigemptyset(&sa_sigint.sa_mask);
    sa_sigint.sa_sigaction = sigint_handler;
    sa_sigint.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa_sigint, NULL);

    time_t t; 
    struct tm *_tm;

    while(loop) {
        pid_t proc = fork();

        if (proc < 0) {
            fprintf(stderr, "Something went wrong with creating process...\n");
            exit(0);

        } else if (proc == 0) {
            srand(getpid());
            struct sigaction sa_alarm;
            sigemptyset(&sa_alarm.sa_mask);
            sa_alarm.sa_sigaction = sigalarm_handler;
            sa_alarm.sa_flags = SA_SIGINFO;
            sigaction(SIGALRM, &sa_alarm, NULL);

            int ttl = rand() % m + 1;

            time(&t);
            _tm = localtime(&t);
            alarm(ttl);

            printf("[ %d ] [ %d ] [ %02d / %02d / %04d %02d:%02d:%02d ]\n", getpid(), ttl, 
                    _tm->tm_mday, _tm->tm_mon + 1, _tm->tm_year + 1900, _tm->tm_hour, _tm->tm_min, _tm->tm_sec
            );

            unsigned long long res = inf_factorial();
            exit(ttl);
        } else {
            number_of_process++;
        }
        _sleep(w);
    } 

    while(number_of_process > 0) {
        //wait(NULL);
        //int status;
        //waitpid(-1, &status, 0);
        pause();
        //number_of_process--;
    }
    return 0;
}
