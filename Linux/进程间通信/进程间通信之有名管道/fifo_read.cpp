#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#define FIFO_PATH "/tmp/myfifo"

int main() {
    int fd = open(FIFO_PATH, O_RDONLY);
    char buffer[100];
    printf("接收前\n");
    read(fd, buffer, sizeof(buffer));
    printf("Received: %s\n", buffer);
    close(fd);
    unlink(FIFO_PATH);  // 清理管道文件
    while(1)
    {
        usleep(10000);
    }
    return 0;
}