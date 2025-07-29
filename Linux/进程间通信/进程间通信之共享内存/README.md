共享内存是 Linux 中最快的进程间通信（IPC）机制，允许多个进程直接访问同一块物理内存区域，避免了数据复制的开销。其核心原理是通过内存映射，使不同进程的虚拟地址空间映射到相同的物理内存页。以下是详细解析和代码示例：

⚙️ 一、核心原理

1. 物理内存共享  
   多个进程通过系统调用将同一块物理内存映射到各自的虚拟地址空间，进程读写操作直接作用于物理内存，无需内核中转。
2. 零数据复制  
   与管道、消息队列等需要数据拷贝的 IPC 不同，共享内存只需两次拷贝（数据从源到共享内存、从共享内存到目标），大幅提升效率。
3. 缺乏内置同步  
   共享内存本身不提供互斥机制，需配合信号量、互斥锁等同步工具，否则可能发生数据竞争。

🔧 二、三种实现方式对比

实现方式 System V 共享内存 POSIX 共享内存 mmap 文件映射

关键函数 shmget、shmat、shmdt shm_open、mmap mmap、munmap

标识符 key_t (通过 ftok 生成) 文件路径（如 /my_shm） 文件描述符（普通文件）

持久性 进程退出后仍存在 进程退出后仍存在 文件删除后失效

同步要求 需额外信号量 需额外信号量 需额外信号量

适用场景 传统 IPC 现代应用，更简洁 需文件备份的共享场景

🛠️ 三、实现步骤（以 System V 为例）

1. 创建共享内存段
   ```
   #include <sys/ipc.h>
   #include <sys/shm.h>
   
   key_t key = ftok("/tmp/shmfile", 65); // 生成唯一 key
   int shmid = shmget(key, 1024, IPC_CREAT | 0666); // 创建 1KB 共享内存
   
   • key：唯一标识符，可用 IPC_PRIVATE 自动生成。

   • size：共享内存大小（建议 4KB 对齐）。

3. 映射到进程地址空间  
   char *shm_ptr = (char*)shmat(shmid, NULL, 0); // 映射到进程虚拟地址
   
   • 返回指针 shm_ptr 指向共享内存起始地址。

4. 读写数据  
   strcpy(shm_ptr, "Hello, Shared Memory!"); // 写入数据
   printf("Read: %s\n", shm_ptr);            // 读取数据
   

5. 解除映射  
   shmdt(shm_ptr); // 断开连接
   

6. 删除共享内存  
   shmctl(shmid, IPC_RMID, NULL); // 标记删除（实际在所有进程分离后释放）
   

💻 四、代码示例

场景：生产者-消费者模型（System V）

公共头文件 shm_com.h  
```
#include <sys/shm.h>
#define TEXT_SZ 256

struct shared_data {
    int written;        // 同步标志：1 表示有新数据
    char text[TEXT_SZ]; // 数据缓冲区
};
```

生产者进程（writer.c）  
```
#include "shm_com.h"
int main() {
    int shmid = shmget((key_t)1234, sizeof(struct shared_data), 0666 | IPC_CREAT);
    struct shared_data *shm_ptr = (struct shared_data*)shmat(shmid, NULL, 0);
    
    shm_ptr->written = 0;
    strcpy(shm_ptr->text, "Data from Producer");
    shm_ptr->written = 1; // 通知消费者可读

    shmdt(shm_ptr);
    shmctl(shmid, IPC_RMID, 0); // 删除共享内存
}
```


消费者进程（reader.c）  
```
#include "shm_com.h"
int main() {
    int shmid = shmget((key_t)1234, sizeof(struct shared_data), 0666);
    struct shared_data *shm_ptr = (struct shared_data*)shmat(shmid, NULL, 0);
    
    while (shm_ptr->written == 0) sleep(1); // 轮询等待新数据
    printf("Received: %s\n", shm_ptr->text);
    
    shmdt(shm_ptr);
}
```

POSIX 共享内存示例

// 写进程
```
int shm_fd = shm_open("/my_shm", O_CREAT | O_RDWR, 0666);
ftruncate(shm_fd, 1024);
char *ptr = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
strcpy(ptr, "POSIX Shared Memory");
munmap(ptr, 1024);
shm_unlink("/my_shm"); // 删除共享对象
```

编译需链接 -lrt 库。

⚠️ 五、关键注意事项

1. 同步机制  
   • 示例中通过 written 标志轮询，实际面试中需强调这是低效做法，生产环境必须用信号量（sem_init）或互斥锁。

2. 生命周期管理  
   • 共享内存段不随进程退出自动释放，必须显式删除（shmctl 或 shm_unlink），否则会导致内存泄漏。

3. 权限与大小限制  
   • 单个共享内存段大小受 /proc/sys/kernel/shmmax 限制（默认 32MB），可通过 echo 2147483648 > /proc/sys/kernel/shmmax 调整为 2GB。

4. 错误处理  
   • 所有系统调用需检查返回值（如 shmat 失败返回 (void*)-1），避免未定义行为。

💎 六、面试常见问题

1. 共享内存 vs 其他 IPC 的优劣？  
   • 优势：速度最快（零拷贝）；劣势：需手动同步，存在安全风险（恶意进程可篡改数据）。

2. 共享内存泄漏如何排查？  
   • 命令 ipcs -m 查看所有共享内存段，ipcrm -m [shmid] 删除残留段。

3. 如何保证多进程写入安全？  
   • 使用 System V 信号量 或 POSIX 互斥锁（锁变量需放在共享内存中）。

📚 总结

• 核心价值：高性能数据交换，适合实时系统、高频通信场景。

• 面试要点：  

  • 熟记 System V/POSIX API 流程 → 创建、映射、读写、解绑、删除。  

  • 强调同步必要性（信号量代码建议提前准备）。  

  • 理解生命周期（显式删除）和资源泄漏风险。  

完整代码可参考：http://mp.weixin.qq.com/s?__biz=Mzg3NjY2NDQ5MA==&sn=5e1478d7326a0c24b8d4676ed7bd475f，https://blog.csdn.net/suifengme/article/details/140753861。
