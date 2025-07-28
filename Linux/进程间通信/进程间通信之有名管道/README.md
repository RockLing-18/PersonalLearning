有名管道（Named Pipe），也称为 ​​FIFO（First In, First Out）​​，是 Linux 进程间通信（IPC）的核心机制之一。与匿名管道不同，有名管道通过文件系统路径标识，允许​​任意进程（无需亲缘关系）​​ 进行通信。以下是其原理、实现及代码示例的详细解析，结合面试场景进行优化说明。
--------------------------------------------------------------------------------
⚙️ ​​一、有名管道的核心原理​​
1.​​文件系统可见性​​
有名管道在文件系统中以特殊文件形式存在（文件类型标识符为 p），路径由用户指定（如 /tmp/myfifo）。其数据​​仅存于内存缓冲区​​，不占用磁盘空间。
2.​​先进先出（FIFO）规则​​数据写入顺序与读取顺序严格一致，确保时序性。
3.​​阻塞与非阻塞模式​​

- ​​默认阻塞​​：若读端未打开，写端 open() 阻塞；若写端未打开，读端 open() 阻塞。
- ​​非阻塞模式​​：通过 O_NONBLOCK 标志避免阻塞（如 open("/tmp/myfifo", O_RDONLY | O_NONBLOCK)）。
4.​​原子性写入​​

- 单次写入 ≤ PIPE_BUF（通常 4KB）时，内核保证原子性（数据不交错）。
--------------------------------------------------------------------------------
🛠️ ​​二、实现步骤与关键函数​​
​​1. 创建有名管道​​
使用 mkfifo() 系统调用：
#include <sys/stat.h>
int mkfifo(const char *pathname, mode_t mode);
- ​​参数​​：pathname：管道文件路径（如 "/tmp/myfifo"）；mode：权限（如 0666，实际权限受 umask 影响）。
- ​​返回值​​：成功返回 0，失败返回 -1（错误码如 EEXIST 表示文件已存在）。
​​2. 读写操作​​
- ​​写入端​​：

int fd = open("fifo_path", O_WRONLY);  // 阻塞直到读端打开
write(fd, data, strlen(data));
- ​​读取端​​：

int fd = open("fifo_path", O_RDONLY);  // 阻塞直到写端打开
read(fd, buffer, sizeof(buffer));  

​​3. 关闭与清理​​  

close(fd);          // 关闭文件描述符    

unlink("fifo_path"); // 删除管道文件（避免残留）
--------------------------------------------------------------------------------
💻 ​​三、代码示例​​
​​场景 1：单向通信（写端 → 读端）​​
​​写入端（writer.c）​​：
```
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#define FIFO_PATH "/tmp/myfifo"

int main() {
    mkfifo(FIFO_PATH, 0666);  // 创建管道（若存在则忽略）
    int fd = open(FIFO_PATH, O_WRONLY);
    char *msg = "Hello from Writer!";
    write(fd, msg, strlen(msg) + 1);  // +1 包含字符串结束符
    close(fd);
    return 0;
}
```

​​读取端（reader.c）​​：
```
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#define FIFO_PATH "/tmp/myfifo"

int main() {
    int fd = open(FIFO_PATH, O_RDONLY);
    char buffer[100];
    read(fd, buffer, sizeof(buffer));
    printf("Received: %s\n", buffer);
    close(fd);
    unlink(FIFO_PATH);  // 清理管道文件
    return 0;
}
```
​​场景 2：双向通信（需两个管道）​​
graph LR
    A[进程A] -- 管道1 --> B[进程B]
    B -- 管道2 --> A
- ​​创建两个管道​​：/tmp/fifo_AtoB（A 写 → B 读）/tmp/fifo_BtoA（B 写 → A 读）
- 每个进程需​​同时打开两个管道​​（注意避免死锁）。
--------------------------------------------------------------------------------
⚠️ ​​四、注意事项与面试高频问题​​
1.​​阻塞陷阱​​
- 读端关闭后，写端继续写入会触发 SIGPIPE 信号（默认终止进程）。
- ​​解决方案​​：写端捕获信号或检查 read 返回值（返回 0 表示写端关闭）。
2.​​非阻塞模式适用场景​​
- 需轮询数据时（如实时监控），但需处理 EAGAIN 错误。
3.​​权限控制​​
通过 mode 参数限制访问（如 0600 仅允许所有者读写）。
4.​​与匿名管道的区别​​
​​特性​​有名管道匿名管道​​进程关系​​任意进程亲缘关系进程（父子等）​​生命周期​​显式删除（unlink()）随进程结束自动销毁​​文件系统可见​​✅❌​​双向通信​​需两个管道需两个管道​​使用场景​​不相关进程、持久化通信需求临时快速通信
5.​​性能优化​​
- 避免小数据频繁写入（多次 write 调用增加系统开销）。
- 大文件传输时，需分块读取（如 read(fd, buf, 4096)）。
--------------------------------------------------------------------------------
💎 ​​五、总结​​
- ​​核心价值​​：突破亲缘关系限制，通过文件系统路径实现​​跨进程通信​​。
- ​​适用场景​​：日志收集、多进程任务调度、Shell 命令协作（如 mkfifo + tail）。
- ​​面试要点​​：

- 熟悉 mkfifo/open/write/read 的​​返回值检查​​（如 errno 处理）。
- 理解​​阻塞机制​​和​​原子性写入​​的边界条件（PIPE_BUF 大小）。
- 掌握​​双向通信​​的实现逻辑（避免单管道死锁）。
可通过命令 ls -l /tmp/myfifo 验证管道创建（文件类型显示为 ​​p​​）。实际编码时，建议在程序启动时清理残留管道（access() + unlink()）。
