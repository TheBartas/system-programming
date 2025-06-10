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
#define BUFFER_SIZE 400000

char *get_ext1(const char *url) {
    return url ? strrchr(url, '.') + 1 : NULL;
}

char *get_ext(const char *url) {
    if (!url) return NULL;
    char *dot = strrchr(url, '.');
    if (!dot) return NULL;
    return dot + 1;
}

char *set_ext(char *ext) {
    if (strcmp(ext, "html") == 0) return "html";
    else if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) return "jpg";
    else if (strcmp(ext, "png") == 0) return "png";
    else if (strcmp(ext, "gif") == 0) return "gif";
    else if (strcmp(ext, "ico") == 0) return "ico";

    return "plain";
}

void build_http_response(const char *file_path, char *ext, char *res, size_t *res_len) {
    int file_fd = open(file_path, O_RDONLY);

    if (file_fd < 0) {
        const char *not_found =
            "HTTP/1.0 404 Not Found\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "404 Not Found\n";
        strcpy(res, not_found);
        *res_len = strlen(not_found);
        return;
    }
    
    struct stat st;
    fstat(file_fd, &st); 

    int header_len = snprintf(res, BUFFER_SIZE, 
            "HTTP/1.0 200 OK\r\n"
            "Content-Type: text/%s\r\n\r\n",
            set_ext(ext));

    printf("nigga set_ext %s\n", set_ext(ext));

    ssize_t bytes_read = read(file_fd, res + header_len, BUFFER_SIZE - header_len);
    if (bytes_read < 0) bytes_read = 0;

    *res_len = header_len + bytes_read;
    close(file_fd);
}

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
        buf[matches[1].rm_eo] = '\0';
        printf("From buf: %s\n", buf);
        const char *url_path = buf + matches[1].rm_so;

        if (strlen(url_path) == 0) url_path = "index.html";

        char path[BUFFER_SIZE];
        snprintf(path, BUFFER_SIZE, "html/%s", url_path);

        char *ext = get_ext(url_path);
        printf("ext %s\n", ext);


        char header[256];
        char *response = malloc(BUFFER_SIZE * 2 * sizeof(char));
        size_t res_len;
        build_http_response(path, ext, response, &res_len);
        puts("NIgga");
        // send(client_fd, );
        printf("%ld\n", res_len);
        printf("%s\n", header);
        write(client_fd, response, res_len);
        puts("NIgga after");
        free(response);
    }  else {
        const char *not_implemented =
            "HTTP/1.0 501 Not Implemented\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "501 Not Implemented: Method not supported\n";

        send(client_fd, not_implemented, strlen(not_implemented), 0);
        exit(1);
    }

    regfree(&reg);
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
        exit(EXIT_FAILURE);
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