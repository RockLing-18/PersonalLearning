信号量（Semaphore）是 Linux 中用于进程或线程间同步的核心机制，通过原子操作（P/V）控制对共享资源的访问，避免竞争条件和死锁。以下是其原理、实现方式及代码示例，供面试准备参考。

⚙️ 一、信号量核心原理

1. 基本定义  
   • 信号量是一个整型计数器，表示可用资源的数量：

     ◦ P 操作（sem_wait）：申请资源，计数器减 1；若值为 0 则阻塞等待。

     ◦ V 操作（sem_post）：释放资源，计数器加 1，唤醒等待进程。

   • 原子性：P/V 操作由 CPU 指令（如 CAS）保证原子性，避免并发冲突。

2. 信号量类型  
   • 二元信号量（Binary Semaphore）：  

     计数器仅取值 0 或 1，用于互斥访问（类似互斥锁）。
   • 计数信号量（Counting Semaphore）：  

     计数器为非负整数，限制同时访问资源的进程数（如数据库连接池）。

3. 内核实现机制  
   • 计数器（count）：记录可用资源数量。

   • 等待队列：阻塞的进程加入队列，由自旋锁保护（SMP 环境下）。

🔧 二、Linux 信号量实现方式

1. System V 信号量（进程间同步）

• 特点：支持信号量集，随内核持续，需显式删除。

• 核心函数：

  • semget()：创建/获取信号量集。

  • semop()：执行 P/V 操作。

  • semctl()：控制信号量（初始化、删除等）。

2. POSIX 信号量（线程/进程间同步）

• 特点：更简洁，支持线程同步，可通过共享内存实现进程间同步。

• 核心函数：

  • sem_init()：初始化信号量。

  • sem_wait()/sem_post()：执行 P/V 操作。

  • sem_destroy()：销毁信号量。

💻 三、代码示例

场景：生产者-消费者问题（缓冲区大小 5）

1. POSIX 信号量（多线程）
```
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#define BUFFER_SIZE 5
int buffer[BUFFER_SIZE];
sem_t empty, full;  // 信号量：空槽位、已填充槽位
pthread_mutex_t mutex;  // 互斥锁保护缓冲区

void* producer(void* arg) {
    for (int i = 0; i < 10; i++) {
        sem_wait(&empty);  // P(empty)：等待空槽位
        pthread_mutex_lock(&mutex);
        buffer[i % BUFFER_SIZE] = i;  // 生产数据
        printf("Producer: %d\n", i);
        pthread_mutex_unlock(&mutex);
        sem_post(&full);   // V(full)：增加已填充计数
    }
    return NULL;
}

void* consumer(void* arg) {
    for (int i = 0; i < 10; i++) {
        sem_wait(&full);   // P(full)：等待数据
        pthread_mutex_lock(&mutex);
        int item = buffer[i % BUFFER_SIZE];  // 消费数据
        printf("Consumer: %d\n", item);
        pthread_mutex_unlock(&mutex);
        sem_post(&empty);  // V(empty)：释放空槽位
    }
    return NULL;
}

int main() {
    pthread_t prod, cons;
    sem_init(&empty, 0, BUFFER_SIZE);  // 初始化空槽位为5
    sem_init(&full, 0, 0);             // 初始化已填充为0
    pthread_mutex_init(&mutex, NULL);

    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&mutex);
    return 0;
}
```
编译命令：gcc -pthread prod_cons.c -o prod_cons  
关键点：  
• empty 和 full 信号量协调生产/消费节奏。

• 互斥锁 mutex 保护缓冲区操作。

2. System V 信号量（多进程）
```
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>

// 信号量操作联合体（System V 要求）
union semun { int val; };

void pv(int semid, int op) {  // P/V 操作封装
    struct sembuf sop = {0, op, 0};
    semop(semid, &sop, 1);
}

int main() {
    key_t key = ftok("/tmp", 'S');  // 生成唯一 key
    int semid = semget(key, 1, IPC_CREAT | 0666);  // 创建信号量

    union semun arg;
    arg.val = 1;  // 初始化为1（二元信号量）
    semctl(semid, 0, SETVAL, arg);  // 设置信号量值

    if (fork() == 0) {  // 子进程
        printf("Child waiting...\n");
        pv(semid, -1);  // P操作：获取锁
        printf("Child in critical section\n");
        sleep(2);
        pv(semid, 1);   // V操作：释放锁
        return 0;
    }

    // 父进程
    printf("Parent waiting...\n");
    pv(semid, -1);  // P操作
    printf("Parent in critical section\n");
    sleep(1);
    pv(semid, 1);   // V操作

    wait(NULL);  // 等待子进程结束
    semctl(semid, 0, IPC_RMID);  // 删除信号量
    return 0;
}
```
关键点：  
• semop 执行原子 P/V 操作。

• SEM_UNDO 标志防止进程崩溃导致死锁（示例中未显式使用，实际建议添加）。

⚠️ 四、常见面试问题

1. 信号量 vs 互斥锁  
   • 信号量：可控制多个资源（如允许 N 个进程访问），支持计数。

   • 互斥锁：二元锁，同一时间仅允许一个进程进入临界区。

2. 死锁如何避免？  
   • 有序获取：所有进程按固定顺序申请锁。

   • 超时机制：使用 sem_timedwait() 避免无限阻塞。

3. 信号量泄漏怎么办？  
   • 命令 ipcs -s 查看信号量，ipcrm -s <semid> 手动删除残留资源。

4. 优先级反转问题  
   • 高优先级进程等待低优先级进程释放信号量时，可通过优先级继承协议（如 Linux 的 PIP）解决。

💎 总结

• 核心价值：信号量通过 P/V 原子操作协调多进程/线程对共享资源的访问。

• 面试要点：

  • 原理：计数器 + 等待队列 + 原子操作。

  • 实现选择：线程同步用 POSIX，进程间同步用 System V。

  • 同步场景：生产者-消费者、资源池管理、读写控制。

• 避坑指南：  

  • 初始化信号量值必须正确（二进制为 1，计数按需）。

  • P/V 操作需严格配对，避免死锁或资源泄漏。  

  • 进程退出前调用 semctl(semid, 0, IPC_RMID) 或 sem_destroy() 清理资源。


# semop 与 sem_wait 和 sem_post 的区别
semop 函数是 System V 信号量体系中的核心操作函数，用于原子性地执行信号量的 P（等待）和 V（释放）操作。它与 POSIX 信号量的 sem_wait 和 sem_post 功能相似但属于不同的 API 体系，以下是详细解析：

🔧 一、semop 的作用与机制

1. 基本功能  
   semop 通过一次系统调用，原子性地对信号量集中的信号量执行批量操作（如加减值）。其函数原型如下：
   int semop(int semid, struct sembuf *sops, unsigned nsops);
   
   • semid：信号量集的标识符（由 semget 创建）。

   • sops：指向 sembuf 结构体数组的指针，描述具体操作。

   • nsops：需操作的信号量数量。

2. 操作类型（sembuf 结构体）
   ```
   struct sembuf {
       unsigned short sem_num; // 信号量在集合中的索引
       short sem_op;           // 操作值（负数：P 操作；正数：V 操作）
       short sem_flg;          // 标志位（如 `0`、`IPC_NOWAIT`、`SEM_UNDO`）
   };
   ```
   • P 操作（申请资源）：sem_op = -1，信号量值减 1。若值变为负数，进程阻塞（除非指定 IPC_NOWAIT）。

   • V 操作（释放资源）：sem_op = +1，信号量值加 1，唤醒等待进程。

3. 原子批量操作  
   semop 的独特优势是支持对多个信号量一次性执行操作，避免死锁。  
   示例：银行转账需同时锁定转出账户（信号量 A）和转入账户（信号量 B）：
   struct sembuf ops[2] = {
       {0, -1, 0}, // P 操作：锁定信号量 0
       {1, -1, 0}  // P 操作：锁定信号量 1
   };
   semop(semid, ops, 2); // 原子执行两个 P 操作
   
   若分开执行可能因竞争导致死锁，而 semop 确保所有操作要么全成功，要么全失败。

🔄 二、semop 与 sem_wait/sem_post 的关系

1. 功能等价性

   操作 System V (semop) POSIX
P 操作 semop 设置 sem_op = -1 sem_wait(sem_t *sem)
V 操作 semop 设置 sem_op = +1 sem_post(sem_t *sem)

   • 两者均实现信号量的 PV 原语，用于资源申请与释放。

2. 关键差异

   特性 System V (semop) POSIX (sem_wait/sem_post)
作用对象 信号量集（多个信号量） 单个信号量
原子批量操作 ✅ 支持 ❌ 不支持
复杂度 复杂（需手动管理 sembuf 结构） 简单（直接操作信号量对象）
性能开销 较高（需内核切换） 较低（无竞争时用户态完成）
适用场景 多进程复杂同步（如需原子锁多个资源） 线程间同步或简单进程同步

⚠️ 三、使用注意事项

1. 错误处理  
   semop 失败时返回 -1 并设置 errno，需检查如 EAGAIN（非阻塞模式资源不足）、EINTR（被信号中断）等错误码。

2. SEM_UNDO 标志  
   若设置 sem_flg = SEM_UNDO，内核会在进程异常退出时自动回滚信号量操作，避免资源泄漏。

3. 初始化要求  
   System V 信号量需通过 semctl 初始化值，而 POSIX 信号量直接用 sem_init 或 sem_open。

💎 四、总结

• semop 是 System V 信号量的核心操作函数，通过 sembuf 结构灵活支持批量 PV 操作，尤其适合需原子管理多个信号量的复杂场景（如数据库事务）。

• sem_wait 和 sem_post 属于 POSIX 标准，API 更简洁高效，适用于线程同步或简单进程同步。

• 选型建议：

  • 需跨进程原子操作多个资源 → System V semop  

  • 线程同步或轻量进程同步 → POSIX sem_wait/sem_post  

  • 高性能要求 → 优先选择 POSIX。
