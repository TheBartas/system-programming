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
#include <regex.h>
#include <fcntl.h>
#include <string.h>

#define SERVER_HOST_ADDR "127.0.0.1"
#define BUFFER_SIZE 2048

void build_http_response(const char *file_name, 
                        const char *file_ext, 
                        char *response, 
                        size_t *response_len) {

    const char *mime_type = get_mime_type(file_ext);
    char *header = (char *)malloc(BUFFER_SIZE * sizeof(char));
    snprintf(header, BUFFER_SIZE,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "\r\n",
             mime_type);

    int file_fd = open(file_name, O_RDONLY);
    if (file_fd == -1) {
        snprintf(response, BUFFER_SIZE,
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: text/plain\r\n"
                 "\r\n"
                 "404 Not Found");
        *response_len = strlen(response);
        return;
    }

    struct stat file_stat;
    fstat(file_fd, &file_stat);
    off_t file_size = file_stat.st_size;

    *response_len = 0;
    memcpy(response, header, strlen(header));
    *response_len += strlen(header);

    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, 
                            response + *response_len, 
                            BUFFER_SIZE - *response_len)) > 0) {
        *response_len += bytes_read;
    }
    free(header);
    close(file_fd);
}

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));

    ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (bytes_received > 0) {

        regex_t regex;
        regcomp(&regex, "^GET /([^ ]*) HTTP/1", REG_EXTENDED);
        regmatch_t matches[2];

        if (regexec(&regex, buffer, 2, matches, 0) == 0) {
            // extract filename from request and decode URL
            buffer[matches[1].rm_eo] = '\0';
            const char *url_encoded_file_name = buffer + matches[1].rm_so;
            char *file_name = url_decode(url_encoded_file_name);


            char file_ext[32];
            strcpy(file_ext, "html");


            char *response = (char *)malloc(BUFFER_SIZE * 2 * sizeof(char));
            size_t response_len;
            build_http_response(file_name, file_ext, response, &response_len);

            send(client_fd, response, response_len, 0);

            free(response);
            free(file_name);
        }
        regfree(&regex);
    }
    close(client_fd);
    free(arg);
    free(buffer);
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

    if (q_flag) {
        // TODO: close server if exists
    }
    if (q_flag == 1 && s_flag == 1) {
        fprintf(stderr, "[Error] Cannot use <-s> [%s] and <-q> [%s] in the same command.\n", s_flag ? "true" : "false", q_flag ? "true" : "false");
        exit(-1);
    }

    int ifsd = socket(AF_INET, SOCK_STREAM, 0); // AF_UNIX odwołuje się do plików lokalnych

    struct addrinfo* res;
    int hostinfo = getaddrinfo(SERVER_HOST_ADDR, port, NULL, &res);
    int bnd = bind(ifsd, res->ai_addr, res->ai_addrlen);

    printf("bind = %d\n", bnd);
    if (bnd < 0) {
        printf("Error\n");
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