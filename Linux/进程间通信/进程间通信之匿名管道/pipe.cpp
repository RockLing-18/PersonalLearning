#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    char buffer[1024];
    
    // 1. 创建管道
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // 子进程
        close(pipefd[1]);  // 关闭写端
        ssize_t count = read(pipefd[0], buffer, sizeof(buffer));
        if (count > 0) {
            buffer[count] = '\0';
            std::cout << "子进程收到: " << buffer << std::endl;
        }
        close(pipefd[0]);
        exit(EXIT_SUCCESS);
    } 
    else { // 父进程
        close(pipefd[0]);  // 关闭读端
        const char* msg = "Hello from parent!";
        write(pipefd[1], msg, strlen(msg));
        close(pipefd[1]);
        wait(nullptr); // 等待子进程结束
    }
    return 0;
}