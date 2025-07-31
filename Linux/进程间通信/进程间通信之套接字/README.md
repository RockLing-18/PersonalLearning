套接字（Socket）是 Linux 中用于进程间通信（IPC） 和网络通信的核心机制，支持本地（同一主机）和远程（跨主机）进程的数据交换。其核心原理是通过内核管理的通信端点实现双向数据传输。以下是针对面试准备的系统性解析，包含原理、实现流程及代码示例。

⚙️ 一、套接字核心原理

1. 通信端点模型  
   • 套接字是内核创建的通信端点，表现为文件描述符，进程通过读写该描述符交换数据。

   • 本地通信：使用 AF_UNIX 地址族，数据不走网络协议栈，通过文件系统路径标识（如 /tmp/socket.sock），效率高。

   • 网络通信：使用 AF_INET（IPv4）或 AF_INET6（IPv6），依赖 TCP/IP 协议栈。

2. 套接字类型与协议  
   类型 协议 特点 适用场景
SOCK_STREAM TCP 面向连接、可靠传输、数据有序（字节流），需三次握手建立连接 HTTP、FTP、数据库连接
SOCK_DGRAM UDP 无连接、不可靠、数据独立（数据报），无需建立连接 实时音视频、DNS 查询
SOCK_RAW 自定义 直接访问 IP 层，可构造自定义协议头 网络抓包（如 Wireshark）
SOCK_SEQPACKET SCTP 面向连接、消息边界完整（报文有序），支持多流传输 电信系统、金融交易

3. 通信流程  
   • TCP 流程：  
     graph LR
         S[服务器] -->|1. socket| A[创建套接字]
         A -->|2. bind| B[绑定IP端口]
         B -->|3. listen| C[监听连接]
         C -->|4. accept| D[接受连接]
         D -->|5. read/write| E[数据交换]
         E -->|6. close| F[关闭连接]
         C[客户端] -->|1. socket| G[创建套接字]
         G -->|2. connect| H[连接服务器]
         H -->|3. read/write| E
     
   • UDP 流程：无需连接，直接通过 sendto() 和 recvfrom() 收发数据。

💻 二、代码示例

1. 本地进程间通信（Unix Domain Socket）

服务器端：
```
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    int server_fd, client_fd;
    struct sockaddr_un server_addr;
    char buf[100];

    // 1. 创建套接字
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, "/tmp/socket.sock");

    // 2. 绑定地址
    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    // 3. 监听连接
    listen(server_fd, 5);
    // 4. 接受连接
    client_fd = accept(server_fd, NULL, NULL);
    // 5. 读取数据
    read(client_fd, buf, sizeof(buf));
    printf("Server received: %s\n", buf);
    // 6. 关闭
    close(client_fd);
    close(server_fd);
    unlink("/tmp/socket.sock"); // 删除套接字文件
    return 0;
}
```

客户端：
```
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

int main() {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = "/tmp/socket.sock"
    };
    // 连接服务器
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    // 发送数据
    write(sock, "Hello from client", 17);
    close(sock);
    return 0;
}
```
编译命令：gcc server.c -o server && gcc client.c -o client  
运行顺序：先启动 ./server，再运行 ./client。

2. 网络通信（TCP 套接字）

服务器端：
```
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // 1. 创建套接字
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080), // 端口号
        .sin_addr.s_addr = INADDR_ANY // 监听所有IP
    };
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)); // 2. 绑定
    listen(server_fd, 5); // 3. 监听
    int client_fd = accept(server_fd, NULL, NULL); // 4. 接受连接
    char buf[100];
    read(client_fd, buf, 100); // 5. 读取数据
    printf("Received: %s\n", buf);
    close(client_fd);
    close(server_fd);
    return 0;
}
```

客户端：
```
#include <arpa/inet.h>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr.s_addr = inet_addr("127.0.0.1") // 服务器IP
    };
    connect(sock, (struct sockaddr*)&addr, sizeof(addr)); // 连接服务器
    write(sock, "Hello TCP", 10); // 发送数据
    close(sock);
    return 0;
}
```
说明：  
• htons()：将端口号转换为网络字节序（大端）。  

• INADDR_ANY：服务器绑定到所有可用网络接口。

3. UDP 套接字示例

接收端（服务器）：
```
#include <arpa/inet.h>

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(9090),
        .sin_addr.s_addr = INADDR_ANY
    };
    bind(sock, (struct sockaddr*)&addr, sizeof(addr)); // 绑定端口
    char buf[100];
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    // 接收数据（无连接）
    recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*)&client_addr, &len);
    printf("UDP Received: %s\n", buf);
    close(sock);
    return 0;
}
```

发送端（客户端）：
```
#include <arpa/inet.h>

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(9090),
        .sin_addr.s_addr = inet_addr("127.0.0.1")
    };
    // 直接发送数据（无需连接）
    sendto(sock, "Hello UDP", 10, 0, (struct sockaddr*)&addr, sizeof(addr));
    close(sock);
    return 0;
}
```
关键点：UDP 使用 sendto() 和 recvfrom() 指定目标地址，无连接建立过程。

⚠️ 三、常见面试问题

1. TCP vs UDP 的区别？  
   • TCP：可靠、有序、面向连接（三次握手），适合文件传输、网页访问。  

   • UDP：不可靠、无连接、低延迟，适合实时应用（视频通话）。

2. 为什么 Unix 域套接字比 TCP 本地环回更快？  
   • 数据不经过网络协议栈，直接通过内核缓冲区复制，减少协议解析开销。

3. bind() 失败的可能原因？  
   • 端口被占用（EADDRINUSE）。  

   • 无权限绑定特权端口（<1024 需 root 权限）。

4. 如何处理大量并发连接？  
   • 使用 epoll 或 kqueue（I/O 多路复用），避免多线程/进程的资源开销。

5. 什么是粘包问题？如何解决？  
   • 原因：TCP 是字节流协议，无消息边界。  

   • 方案：定义协议头（包含消息长度），或使用分隔符（如 \n）。

💎 四、总结与对比

|特征|Unix域套接字 | TCP/IP套接字 | UDP/IP 套接字|
|:---:|:-----------:|:-----------:|:------------:|
|通信范围| 同一主机|  跨主机 |     跨主机|
|速度|  ⭐⭐⭐⭐（内核直接复制）| ⭐⭐（协议栈开销）| ⭐⭐⭐（无连接开销）|
|可靠性| 可靠| 可靠（TCP）| 不可靠（UDP）|
|适用场景 |本地服务（MySQL/Nginx）| 远程可靠传输（HTTP）| 实时音视频、广播|

⚠️ 注意事项：  

- 套接字通信后必须关闭描述符，避免资源泄漏。  

- 网络字节序转换（htons/ntohs）是跨平台兼容的关键。  
