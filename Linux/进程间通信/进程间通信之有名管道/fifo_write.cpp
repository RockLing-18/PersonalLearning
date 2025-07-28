#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>  // 包含 mkfifo 函数声明
#include <sys/types.h> // 支持 mode_t 类型
#define FIFO_PATH "/tmp/myfifo"

int main() {
    mkfifo(FIFO_PATH, 0666);  // 创建管道（若存在则忽略）
    int fd = open(FIFO_PATH, O_WRONLY| O_NONBLOCK);
    printf("发送前\n");
    const char *msg = "Hello from Writer!";
    write(fd, msg, strlen(msg) + 1);  // +1 包含字符串结束符
    printf("发送成功\n");
    //close(fd);
    while(1)
    {
        usleep(10000);
    }
    return 0;
}