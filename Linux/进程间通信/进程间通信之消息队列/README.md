以下是针对 Linux 消息队列（Message Queue）的全面解析及代码示例，涵盖核心原理、实现方式和面试要点，帮助您系统掌握该 IPC 机制。

⚙️ 一、消息队列核心原理

1. 内核管理的消息链表  
   • 消息队列由内核维护，进程通过唯一标识符（msqid）访问。

   • 数据以消息节点形式存储，包含类型（mtype）和正文（mtext），支持结构化数据传输。

2. 异步通信机制  
   • 发送方（msgsnd）和接收方（msgrcv）无需同时运行，消息在内核队列中持久化存储（直到显式删除或系统重启）。

3. 消息类型与优先级  
   • mtype 标识消息类别，接收方可按类型选择性读取（如优先处理高优先级消息）。

4. 同步控制  
   • 队列满时 msgsnd 阻塞（除非指定 IPC_NOWAIT）；队列空时 msgrcv 阻塞，实现隐式同步。

📊 二、System V vs POSIX 消息队列对比

特性 System V 消息队列 POSIX 消息队列

标准 传统 UNIX 标准 POSIX 标准（更现代）

标识方式 key_t（通过 ftok 生成） 文件路径（如 /my_queue）

持久性 随内核持续，需显式删除 挂载在 /dev/mqueue，可文件系统管理

函数库 msgget, msgsnd, msgrcv, msgctl mq_open, mq_send, mq_receive

编译选项 无需额外链接 需链接 -lrt

适用场景 跨版本兼容性要求高 新项目开发，需跨平台支持

🛠️ 三、System V 消息队列实现步骤

1. 创建/获取队列
```
#include <sys/ipc.h>
#include <sys/msg.h>

key_t key = ftok("/tmp", 65);  // 生成唯一 Key
int msqid = msgget(key, IPC_CREAT | 0666);  // 创建队列，权限 0666
```
• 关键参数：  

  • IPC_CREAT：不存在时创建队列。  

  • IPC_EXCL：配合 IPC_CREAT，若队列存在则报错。

2. 定义消息结构
```
struct msgbuf {
    long mtype;      // 消息类型（必须 >0）
    char mtext[100]; // 消息正文（可自定义长度）
};
```

3. 发送消息
```
struct msgbuf msg;
msg.mtype = 1;  // 设置消息类型
strcpy(msg.mtext, "Hello, System V Message Queue!");
msgsnd(msqid, &msg, sizeof(msg.mtext), 0);  // 阻塞发送
```
• 非阻塞发送：  
  msgsnd(msqid, &msg, sizeof(msg.mtext), IPC_NOWAIT);  // 队列满时立即返回
  

4. 接收消息
```
struct msgbuf rcv_msg;
// 接收类型为 1 的消息（阻塞模式）
msgrcv(msqid, &rcv_msg, sizeof(rcv_msg.mtext), 1, 0); 
printf("Received: %s\n", rcv_msg.mtext);
```
• 高级接收模式：  

  • msgtyp = 0：读取队列第一条消息。  

  • msgtyp = -n：读取类型 ≤ n 的最高优先级消息。

5. 删除队列

msgctl(msqid, IPC_RMID, NULL);  // 显式删除队列，释放内核资源


💻 四、代码示例

场景 1：基本通信（System V）

发送进程：
```
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

struct msgbuf { long mtype; char text[100]; };

int main() {
    key_t key = ftok("/tmp", 65);
    int msqid = msgget(key, IPC_CREAT | 0666);
    
    struct msgbuf msg = {.mtype = 1};
    strcpy(msg.text, "Data from Sender");
    msgsnd(msqid, &msg, sizeof(msg.text), 0);
    printf("Sent: %s\n", msg.text);
    return 0;
}
```

接收进程：
```
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>

struct msgbuf { long mtype; char text[100]; };

int main() {
    key_t key = ftok("/tmp", 65);
    int msqid = msgget(key, 0666);
    
    struct msgbuf msg;
    msgrcv(msqid, &msg, sizeof(msg.text), 1, 0);  // 接收类型1的消息
    printf("Received: %s\n", msg.text);
    msgctl(msqid, IPC_RMID, NULL);  // 删除队列
    return 0;
}
```

场景 2：优先级消息处理（System V）

// 发送高优先级消息（类型值越小优先级越高）
```
struct msgbuf urgent = {.mtype = 1, .text = "URGENT!"};
struct msgbuf normal = {.mtype = 2, .text = "Normal"};

msgsnd(msqid, &normal, sizeof(normal.text), 0);
msgsnd(msqid, &urgent, sizeof(urgent.text), 0);

// 接收时优先处理类型1的消息
msgrcv(msqid, &rcv_msg, sizeof(rcv_msg.text), -1, 0);  // 接收类型≤1的最高优先级
```

场景 3：POSIX 消息队列
```
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>

#define QUEUE_NAME "/posix_queue"

int main() {
    mqd_t mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0644, NULL);
    mq_send(mq, "Hello POSIX", 12, 0);  // 发送消息

    char buf[100];
    mq_receive(mq, buf, sizeof(buf), NULL);  // 接收消息
    printf("Received: %s\n", buf);

    mq_close(mq);
    mq_unlink(QUEUE_NAME);  // 删除队列
    return 0;
}
```
• 编译：gcc -lrt posix_mq.c -o posix_mq

⚠️ 五、关键注意事项

1. 消息大小限制  
   • 单条消息最大长度由 /proc/sys/kernel/msgmax 控制（默认 8192 字节）。

2. 队列容量管理  
   • 最大消息数：/proc/sys/kernel/msgmnb（默认 16384）。

   • 监控命令：  
     ipcs -q          # 查看所有消息队列
     ipcrm -q <msqid> # 删除残留队列
     
3. 资源泄漏风险  
   • 未删除的消息队列会永久占用内核资源，需确保进程退出前调用 msgctl(msqid, IPC_RMID, NULL)。

💎 六、面试高频问题

1. 消息队列 vs 管道/共享内存？  
   • 优势：支持异步、消息类型过滤、持久化。  

   • 劣势：速度低于共享内存（需内核拷贝），结构比管道复杂。

2. 如何保证消息不丢失？  
   • 启用持久化存储（如 System V 消息队列默认持久化到内核），或结合数据库备份。

3. 多进程竞争如何处理？  
   • System V 隐式同步（阻塞操作）；POSIX 需显式锁（如互斥锁）。

4. 消息顺序混乱怎么办？  
   • 为消息添加序列号，接收端按序号重组。

最佳实践：优先使用 POSIX 消息队列（接口简洁），但需注意 Linux 默认限制（可通过 sysctl 调整 /proc/sys/fs/mqueue/ 下的参数）。
