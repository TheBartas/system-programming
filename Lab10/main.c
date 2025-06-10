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
#include <arpa/inet.h>
#include <dirent.h>
#include <signal.h>

#define SERVER_HOST_ADDR "127.0.0.1"
#define BUFFER_SIZE 4092
#define FLOG_SIZE 256
#define TIME_SIZE 64

pthread_mutex_t mtxwrt = PTHREAD_MUTEX_INITIALIZER;
static char log_file[FLOG_SIZE];

typedef struct {
    int client_fd;
    char *client_dir;
} client_data_t;

void kill_deamon();
void create_deamon();
void validate_flags(int, int, int, int, char*);
char *get_ext(const char*);
void init_log_file(char*);
void log_write(int, char*, char*);
void *handle_client(void*);
void show_options(); 

void show_options() {
    printf("----------------------------------------\n");
    printf("  -s       \trun server [deamon]\n");
    printf("  -q       \tkill server [deamon]\n");
    printf("  -d <dir> \tset root directory\n");
    printf("  -p <port>\tset port [number]\n");
    printf("----------------------------------------\n");
}

void validate_flags(int q_flag, int s_flag, int p_flag, int d_flag, char *port) {
    if (q_flag == 1 && s_flag == 1) {
        fprintf(stderr, "[Error] Cannot use <-s> [%s] and <-q> [%s] in the same command.\n", s_flag ? "true" : "false", q_flag ? "true" : "false");
        exit(-1);
    } 

    if (q_flag == 1) {
        printf("Kill the deamon!\n");
        kill_deamon();
        exit(0);
    }

    if (!p_flag) {
        fprintf(stderr, "[Error] No port specified. Use -p <port>.\n");
        exit(-1);
    }

    if (!d_flag) {
        fprintf(stderr, "[Error] No directory specified. Use -d <dir>.\n");
        exit(-1);
    }

    if (port && atoi(port) < 0) {
        fprintf(stderr, "[Error] Port <%s> cannot be less than 0.\n", port);
        exit(-1);
    }
}

char *get_ext(const char *url) {
    if (!url) return NULL;
    char *dot = strrchr(url, '.');
    printf("Dot: %s\n", dot);
    if (!dot) return "plain";
    return dot + 1;
}

void init_log_file(char *name) {
    const char *slash = strrchr(name, '/');
    snprintf(log_file, sizeof(log_file), "%s.log", slash ? slash + 1 : name);
}

void log_write(int client_fd, char *res, char *path) {
    FILE *f = fopen(log_file, "a+"); 

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    getpeername(client_fd, (struct sockaddr*)&addr, &addr_len);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), client_ip, INET_ADDRSTRLEN);

    time_t now = time(NULL);
    struct tm *_tm = localtime(&now);
    char _time[TIME_SIZE];
    strftime(_time, sizeof(_time), "%Y-%m-%d %H:%M:%S", _tm);

    pthread_mutex_lock(&mtxwrt);
    fprintf(f, "[%s] %s \t %ld \t [%s] \t\t %s\n", _time, client_ip, pthread_self(), res, path);
    pthread_mutex_unlock(&mtxwrt);
    fclose(f);
}

void create_deamon() {
    int nochdir = 1, noclose = 0;
    if(daemon(nochdir, noclose) == -1) {
        perror("deamon");
        exit(EXIT_FAILURE);
    }

    FILE *f = fopen("deamon.pid", "w");
    if (f) {
        fprintf(f, "%d\n", getpid());
        fflush(f);
        fclose(f);
    }
    printf("Daemon PID: %d\n", getpid());
    printf("Deamon has been created! You need enough power (magic attack) to kill him.\n");
}

void kill_deamon() {
    FILE *sword = fopen("deamon.pid", "r");

    char *pid = NULL;
    size_t len = 0;

    if (getline(&pid, &len, sword) < 0)
        perror("Hero! Too less power to kill this deamon you have!\n");
    printf("PID: %s", pid);

    if (kill(atoi(pid), 0) == 0)
        kill(atoi(pid), SIGKILL);
    else 
        perror("Boy, this deamon is already killed!\n");
    
    free(pid);
    fclose(sword);
}

void *handle_client(void *arg) {
    client_data_t *data = (client_data_t *)arg;
    int client_fd = data->client_fd;
    const char *dir = data->client_dir;
    char buf[BUFFER_SIZE];
    
    size_t received = read(client_fd, buf, BUFFER_SIZE);

    if (received < 0) {
        puts("Something went wrong!");
        return NULL;
    }

    regex_t reg;
    regcomp(&reg, "^GET /([^ ]*) HTTP/1.1", REG_EXTENDED);
    regmatch_t matches[2];


    if (regexec(&reg, buf, 2, matches, 0) == 0) {
        buf[matches[1].rm_eo] = '\0';
        const char *url_path = buf + matches[1].rm_so;

        if (strlen(url_path) == 0) url_path = "index.html";

        char path[BUFFER_SIZE];
        snprintf(path, BUFFER_SIZE, "%s/%s", dir, url_path);

        size_t res_len;

        int file_fd = open(path, O_RDONLY);

        if (file_fd < 0) {
            const char *not_found =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/plain\r\n\r\n"
                "404 Not Found\n";
            write(client_fd, not_found, strlen(not_found));
            log_write(client_fd, "404 Not Found", path);
            close(client_fd);
            return NULL;
        }

        struct stat st;
        fstat(file_fd, &st); 


        char *response = NULL;
        response = malloc((BUFFER_SIZE + st.st_size) * sizeof * response);

        int header_len = snprintf(response, BUFFER_SIZE, 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/%s\r\n\r\n",
            get_ext(url_path));

        ssize_t bytes_read = read(file_fd, response + header_len, st.st_size);
        if (bytes_read < 0) bytes_read = 0;

        res_len = header_len + bytes_read;

        write(client_fd, response, res_len);
        log_write(client_fd, "200 OK", path);
        close(client_fd);
        free(response);

    }  else {
        const char *not_implemented =
            "HTTP/1.1 501 Not Implemented\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "501 Not Implemented: Method not supported\n";
        write(client_fd, not_implemented, strlen(not_implemented));
        char method[BUFFER_SIZE] = "-";

        char *space = strchr(buf, ' ');
        if (space && (space - buf) < BUFFER_SIZE) {
            size_t len = space - buf;
            strncpy(method, buf, len);
            method[len] = '\0';
        }

        log_write(client_fd, "501 Not Implemented", method);
        close(client_fd);
    }

    regfree(&reg);
    return NULL;
}

int main(int argc, char *argv[]) {
    const char *optstring = "sqp:d:i";
    int s_flag = 0, q_flag = 0, p_flag = 0, d_flag = 0, ret;
    char * port = NULL; 
    char *dir = NULL, *hash = NULL;

    while ((ret = getopt(argc, argv, optstring)) != -1) {
        switch (ret) {
            case 's':s_flag = 1; break;
            case 'q':q_flag = 1; break;
            case 'p':p_flag = 1; port = optarg; break;
            case 'd':d_flag = 1; dir = optarg; break;
            case 'i':show_options(); return;
            case '?': return -1;
            default: abort();
        }
    }

    printf("s_flag = %d\n", s_flag);
    printf("q_flag = %d\n", q_flag);

    validate_flags(q_flag, s_flag, p_flag, d_flag, port);

    printf("dir: %s\n", dir);
    
    create_deamon();

    init_log_file(argv[0]);

    int ifsd = socket(AF_INET, SOCK_STREAM, 0); // AF_UNIX odwołuje się do plików lokalnych

    int optval = 1;
    socklen_t optlen = sizeof(optval);
    setsockopt(ifsd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);

    struct addrinfo* res;
    int hostinfo = getaddrinfo(SERVER_HOST_ADDR, port, NULL, &res);

    if (bind(ifsd, res->ai_addr, res->ai_addrlen) < 0) {
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

        client_data_t *cltdata = malloc(sizeof(client_data_t));
        cltdata->client_fd = *client_fd;
        cltdata->client_dir = dir;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, (void*)cltdata);
        pthread_detach(tid);
    }

    freeaddrinfo(res);
    return 0;
}