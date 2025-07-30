以下是针对 Linux 进程间通信之文件通信的详解，结合面试场景优化，涵盖原理、实现流程、代码示例及高频考点。

⚙️ 一、核心原理

1. 共享文件作为通信媒介  
   多个进程通过读写同一个文件实现数据交换：
   • 写入进程：将数据写入文件（如 /tmp/ipc_file）。

   • 读取进程：从同一文件读取数据。

   • 本质：让不同进程看到同一份磁盘资源。

2. 数据持久化优势  
   文件通信支持持久化存储，进程退出后数据仍保留，适合需长期保存的通信场景（如日志记录）。

3. 同步挑战  
   文件本身不提供同步机制，需额外手段（如文件锁）避免并发写入导致的数据竞态。

📝 二、实现步骤与函数

1. 文件操作基础函数

函数 作用 示例调用

open() 打开/创建文件 open("/tmp/ipc_file", O_RDWR)

write() 写入数据到文件 write(fd, "data", 4)

read() 从文件读取数据 read(fd, buf, 100)

close() 关闭文件描述符 close(fd)

flock()/fcntl() 文件锁（同步关键） fcntl(fd, F_SETLK, &flock)

2. 文件通信流程

graph TD
    A[进程A] -->|1. 打开文件| F[/tmp/ipc_file]
    F -->|2. 写入数据| A
    B[进程B] -->|3. 打开同一文件| F
    F -->|4. 读取数据| B
    A & B -->|5. 文件锁同步| L[文件锁]


💻 三、代码示例（带同步机制）

场景：生产者-消费者模型（文件通信 + 文件锁）

生产者进程（producer.c）
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>  // 文件锁头文件

int main() {
    const char *file = "/tmp/ipc_file";
    int fd = open(file, O_RDWR | O_CREAT, 0666);  // 创建或打开文件
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    // 加写锁（阻塞直到获取锁）
    struct flock fl = {
        .l_type = F_WRLCK,    // 写锁
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0            // 锁定整个文件
    };
    fcntl(fd, F_SETLKW, &fl);  // 阻塞等待锁

    // 写入数据
    const char *msg = "Producer Data\n";
    write(fd, msg, strlen(msg));
    printf("Producer: Wrote data\n");

    // 释放锁
    fl.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &fl);
    close(fd);
    return 0;
}
```

消费者进程（consumer.c）
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>

int main() {
    const char *file = "/tmp/ipc_file";
    int fd = open(file, O_RDWR);  // 打开同一文件
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    // 加读锁
    struct flock fl = {
        .l_type = F_RDLCK,    // 读锁
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0
    };
    fcntl(fd, F_SETLKW, &fl);  // 阻塞等待锁

    // 读取数据
    char buf[1024];
    ssize_t bytes = read(fd, buf, sizeof(buf));
    buf[bytes] = '\0';
    printf("Consumer: Read data: %s", buf);

    // 释放锁
    fl.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &fl);
    close(fd);
    return 0;
}
```

编译与运行

gcc producer.c -o producer
gcc consumer.c -o consumer
./producer  # 先运行生产者写入数据
./consumer  # 再运行消费者读取数据


输出


Producer: Wrote data
Consumer: Read data: Producer Data


⚠️ 四、关键注意事项

1. 同步机制  
   • 文件锁：使用 fcntl 或 flock 实现互斥访问，避免数据覆盖。

   • 原子写入：单次写入 ≤ PIPE_BUF（默认 4096 字节）时保证原子性，无需额外锁。

2. 性能瓶颈  
   • 磁盘 I/O 开销：频繁读写文件效率低于内存型 IPC（如共享内存）。

   • 适用场景：适合低频通信或需持久化的场景（配置同步、日志收集）。

3. 文件清理  
   • 通信完成后手动删除文件：unlink("/tmp/ipc_file")。

   • 避免残留文件占用磁盘空间。

4. 文件类型选择  
   • 临时文件：存于 /tmp，系统重启自动清除。

   • 持久文件：存于用户目录，长期保留数据。

💎 五、面试高频问题

1. 文件通信 vs 共享内存？  
   特性 文件通信 共享内存
速度 慢（磁盘 I/O） 快（直接内存访问）
持久化 ✅ ❌（进程退出消失）
同步难度 需文件锁 需信号量/互斥锁
适用场景 低频、持久化数据交换
 高频、实时数据交换    。

2. 如何保证写入原子性？  
   • 单次写入 ≤ PIPE_BUF（4096 字节）时，内核保证原子性。

   • 大文件写入需拆分+校验（如 MD5）。

3. 文件锁会阻塞进程吗？  
   • F_SETLKW：阻塞直到获取锁。

   • F_SETLK：非阻塞，失败立即返回 EAGAIN。

4. 多个进程同时读文件需要锁吗？  
   • 不需要（读操作不冲突）。

   • 若涉及“读-修改-写”操作，仍需锁保护。

🔧 六、扩展优化

• 内存映射文件：  

  使用 mmap 将文件映射到内存，避免 read/write 系统调用，提升性能：
  void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  memcpy(addr, data, len);  // 直接操作内存
  munmap(addr, size);
  
• 日志框架集成：  

  结合 rsyslog 或自定义日志轮转，避免文件无限增大。

💡 总结（面试要点）

1. 核心原理：文件作为共享媒介，通过读写操作交换数据。
2. 同步必做：文件锁（fcntl）是避免竞态的关键。
3. 适用场景：持久化数据、低频通信、跨主机兼容（NFS）。
4. 原子性：小数据（≤4KB）写入天然原子，大数据需拆分。
5. 性能认知：明确磁盘 I/O 是效率瓶颈，高频场景选共享内存。

完整代码示例可参考：https://www.sohu.com/a/849235647_121798711。
