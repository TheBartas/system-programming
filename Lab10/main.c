#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <regex.h>

#define SERVER_HOST_ADDR "127.0.0.1"
#define BUFFER_SIZE 2048

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    char buf[BUFFER_SIZE];
    
    size_t received = recv(client_fd, buf, BUFFER_SIZE, 0);

    if (received < 0) {
        puts("Something went wrong!");
        return NULL;
    }

    regex_t reg;
    regcomp(&reg, "^GET /([^ ]*) HTTP/1", REG_EXTENDED);
    regmatch_t matches[2];

    if (regexec(&reg, buf, 2, matches, 0) == 0) {
        buf[matches[1].rm_eo - 1] = '\0';
        printf("From buf: %s\n", buf);
        const char *url_encoded_file_name = buf + matches[1].rm_so;
    }






    return NULL;
}

int main(int argc, char *argv[]) {
    const char *optstring = "sqp:d:";
    int s_flag = 0, q_flag = 0, p_flag = 0, d_flag = 0, ret;
    char * port = NULL; 
    char *dir = NULL, *hash = NULL;

    while ((ret = getopt(argc, argv, optstring)) != -1) {
        switch (ret) {
            case 's':s_flag = 1; break;
            case 'q':q_flag = 1; break;
            case 'p':p_flag = 1; port = optarg; break;
            case 'd':d_flag = 1; dir = optarg; break;
            case '?': return -1;
            default: abort();
        }
    }

    printf("s_flag = %d\n", s_flag);
    printf("q_flag = %d\n", q_flag);

    if (q_flag == 1 && s_flag == 1) {
        fprintf(stderr, "[Error] Cannot use <-s> [%s] and <-q> [%s] in the same command.\n", s_flag ? "true" : "false", q_flag ? "true" : "false");
        exit(-1);
    }

    int ifsd = socket(AF_INET, SOCK_STREAM, 0); // AF_UNIX odwołuje się do plików lokalnych

    if (q_flag) {
        // TODO: close server if exists
        close(ifsd);
    }

    struct addrinfo* res;
    int hostinfo = getaddrinfo(SERVER_HOST_ADDR, port, NULL, &res);
    int bnd = bind(ifsd, res->ai_addr, res->ai_addrlen);

    printf("bind = %d\n", bnd);
    if (bnd < 0) {
        perror("bind");
        exit(-1);
    }

    if (listen(ifsd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int *client_fd = malloc(sizeof(int));

        if ((*client_fd = accept(ifsd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
            perror("accept failed\n");
            continue;
        }


        printf("Connected!\n");

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, (void *)client_fd);
        pthread_detach(tid);
    }

    freeaddrinfo(res);
    return 0;
}