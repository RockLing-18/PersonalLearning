#include "shm_sem.h"
#include <string>

int main() {
    key_t key = ftok(PATHNAME, 'a');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // 创建共享内存
    int shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // 映射共享内存
    char *shm_ptr = (char *)shmat(shmid, NULL, 0);
    if (shm_ptr == (char *)-1) {
        perror("shmat");
        exit(1);
    }

    // 创建信号量并初始化为 1（可用状态）
    int semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }
    init_sem(semid, 1);  // 信号量初始值 = 1

    int i = 0;
    while(true)
    {
        // 生产数据（写入共享内存）
        P(semid);  // 获取锁
        std::string data = "Data from Producer [" + std::to_string(i) +"]";
        const char *msg = "Data from Producer";
        strncpy(shm_ptr, data.c_str(), SHM_SIZE);
        printf("Producer: Wrote '%s'\n", data.c_str());
        V(semid);  // 释放锁
        ++i;
        sleep(2);  // 模拟数据处理耗时
    }
   

    sleep(2);  // 模拟数据处理耗时

    // 清理资源
    shmdt(shm_ptr);
    shmctl(shmid, IPC_RMID, NULL);       // 删除共享内存
    semctl(semid, 0, IPC_RMID, NULL);    // 删除信号量
    return 0;
}