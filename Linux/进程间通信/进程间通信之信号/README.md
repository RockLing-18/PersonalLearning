以下是针对 Linux 进程间通信之信号机制的详解，结合面试场景优化，涵盖原理、流程、实现方式及代码示例：

⚙️ 一、信号的核心原理

1. 异步事件通知  
   信号是一种软件中断，用于异步通知进程发生了特定事件（如用户输入 Ctrl+C 触发的 SIGINT）。  
2. 轻量级通信  
   不携带数据，仅通过信号编号传递事件类型（共 62 个信号，前 31 个为常规信号）。  
3. 内核中转机制  
   发送方通过 kill() 或 raise() 发送信号 → 内核将信号加入目标进程的待处理队列 → 目标进程从内核态返回用户态时处理信号。  

🔧 二、信号的生命周期与关键函数

1. 信号产生方式

来源 示例 信号示例

用户终端操作 Ctrl+C 中断进程 SIGINT (2)

硬件异常 除零错误、非法内存访问 SIGFPE (8), SIGSEGV (11)

系统事件 定时器到期、子进程退出 SIGALRM (14), SIGCHLD (17)

进程主动发送 kill() 系统调用或 kill 命令 任意信号

2. 信号处理函数注册

• signal()：基础注册（可能丢失信号）  
  #include <signal.h>
  void handler(int sig) { /* 处理逻辑 */ }
  signal(SIGINT, handler);  // 注册 SIGINT 处理函数
  
• sigaction()：推荐使用（支持阻塞、重入控制）  
  struct sigaction sa;
  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);  // 清空阻塞信号集
  sigaction(SIGINT, &sa, NULL);  // 注册并指定行为
  

3. 信号发送函数

• kill()：向指定 PID 发送信号  
  kill(pid, SIGTERM);  // 终止目标进程
  
• raise()：向自身发送信号  
  raise(SIGABRT);  // 终止自身并生成 core 文件
  

4. 信号处理行为

行为 说明 宏定义

终止进程 默认处理方式 SIG_DFL

忽略信号 丢弃信号不处理 SIG_IGN

捕获并处理 执行自定义函数 用户函数指针

暂停/继续 暂停进程（SIGSTOP）或继续（SIGCONT） -

💻 三、代码示例

场景 1：父子进程间信号通信
```
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

void child_handler(int sig) {
    if (sig == SIGUSR1) 
        printf("Child: Received SIGUSR1\n");
}

int main() {
    pid_t pid = fork();
    if (pid == 0) { 
        // 子进程注册信号处理
        signal(SIGUSR1, child_handler);
        while(1) pause();  // 等待信号
    } else { 
        // 父进程发送信号
        sleep(1);
        printf("Parent: Sending SIGUSR1 to child\n");
        kill(pid, SIGUSR1);  // 向子进程发信号
        sleep(1);
        kill(pid, SIGTERM);  // 终止子进程
    }
    return 0;
}
```
输出：

Parent: Sending SIGUSR1 to child
Child: Received SIGUSR1


场景 2：可靠信号处理（sigaction）
```
#include <stdio.h>
#include <signal.h>

void handler(int sig, siginfo_t *info, void *context) {
    printf("Received signal %d from PID %d\n", sig, info->si_pid);
}

int main() {
    struct sigaction sa;
    sa.sa_sigaction = handler;  // 使用高级处理函数
    sa.sa_flags = SA_SIGINFO;    // 携带发送者信息
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);

    printf("PID %d waiting for signal...\n", getpid());
    while(1) pause();  // 等待信号
}
```
关键点：  
• SA_SIGINFO 标志允许获取发送者 PID 等额外信息。  

• sigemptyset() 确保处理函数执行期间不阻塞其他信号。

⚠️ 四、面试高频问题与注意事项

1. 信号丢失与可靠性  
   • 常规信号（1~31）不支持排队，连续发送时可能丢失（仅递送一次）。  

   • 解决方案：使用实时信号（SIGRTMIN 以上）并设置 SA_SIGINFO。

2. 信号处理函数的安全限制  
   • 处理函数中避免调用非异步安全函数（如 printf, malloc），可能引发死锁。  

   • 替代方案：仅设置标志位，主循环中处理逻辑（参考 统一事件源模型）。

3. 信号屏蔽与同步  
   • 通过 sigprocmask() 阻塞信号，确保关键代码段不被中断：  
     sigset_t set;
     sigemptyset(&set);
     sigaddset(&set, SIGINT);
     sigprocmask(SIG_BLOCK, &set, NULL);  // 阻塞 SIGINT
     /* 临界区代码 */
     sigprocmask(SIG_UNBLOCK, &set, NULL); // 解除阻塞
     

4. 僵尸进程清理  
   • 父进程注册 SIGCHLD 处理函数，调用 wait() 回收子进程资源：  
     void cleanup(int sig) { while (waitpid(-1, NULL, WNOHANG) > 0); }
     signal(SIGCHLD, cleanup);
     

5. 与其它 IPC 的对比  
   特性 信号 管道/共享内存
数据携带 无 支持大数据传输
异步性 ✅ ❌（需同步机制）
跨进程关系 任意进程 通常需亲缘关系
适用场景 事件通知、异常处理 数据交换

💎 五、总结与面试要点

• 核心价值：信号是最高效的轻量级事件通知机制，适合进程控制、异常处理等场景。  

• 必考知识点：  

  • 信号生命周期（产生→注册→递送→处理）。  

  • signal() vs sigaction() 的可靠性差异。  

  • 信号安全编程（避免处理函数中的副作用）。  

• 扩展方向：  

  • 实时信号的应用（SIGRTMIN + N）。  

  • 信号与多线程的交互（pthread_sigmask）。  
