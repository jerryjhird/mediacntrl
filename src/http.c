#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define INITIAL_BUF 1024

// http.c expected to be included after definition of eval_command
int eval_command(const char *player_hint, const char *cmd_str, const char *arg1);

void start_http_server(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return;
    }

    printf("Mediacntrl HTTP serving on port %d\n", port);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) continue;

        size_t buf_size = INITIAL_BUF;
        char *buf = malloc(buf_size);
        if (!buf) {
            close(client_fd);
            continue;
        }

        ssize_t received = 0;
        while (1) {
            ssize_t n = recv(client_fd, buf + received, buf_size - received - 1, 0);
            if (n <= 0) break;
            received += n;
            buf[received] = '\0';

            if (strstr(buf, "\r\n\r\n")) break;

            if ((size_t)received >= buf_size - 1) {
                buf_size *= 2;
                char *new_buf = realloc(buf, buf_size);
                if (!new_buf) break;
                buf = new_buf;
            }
        }

        // example: GET /playerhint/command/arg HTTP/1.1
        if (received > 5 && strncmp(buf, "GET /", 5) == 0) {
            char *path = buf + 5;
            char *end_of_path = strchr(path, ' ');
            if (end_of_path) *end_of_path = '\0';

            char *p_hint = NULL, *c_str = NULL, *a1 = NULL;

            p_hint = path;
            char *slash1 = strchr(p_hint, '/');
            if (slash1) {
                *slash1 = '\0';
                c_str = slash1 + 1;
                
                char *slash2 = strchr(c_str, '/');
                if (slash2) {
                    *slash2 = '\0';
                    a1 = slash2 + 1;
                }
            }

            if (p_hint && *p_hint != '\0' && c_str && *c_str != '\0') {
                eval_command(p_hint, c_str, a1); // run command
                
                const char *ok = "HTTP/1.1 200 OK\r\n"
                                 "Content-Type: text/plain\r\n"
                                 "Content-Length: 3\r\n"
                                 "Connection: close\r\n\r\nOK\n";
                send(client_fd, ok, strlen(ok), 0);
            } else {
                const char *err = "HTTP/1.1 400 Bad Request\r\n"
                                  "Content-Length: 22\r\n"
                                  "Connection: close\r\n\r\nUsage: /player/command";
                send(client_fd, err, strlen(err), 0);
            }
        }

        free(buf);
        close(client_fd);
    }
}