#include "shm_sem.h"

int main() {
    key_t key = ftok(PATHNAME, 'a');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // 获取共享内存
    int shmid = shmget(key, SHM_SIZE, 0666);
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

    // 获取信号量
    int semid = semget(SEM_KEY, 1, 0666);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }

    while(true)
    {
        // 消费数据（读取共享内存）
        P(semid);  // 获取锁
        printf("Consumer: Received '%s'\n", shm_ptr);
        V(semid);  // 释放锁
        sleep(1);
    }
   

    // 清理资源
    shmdt(shm_ptr);
    return 0;
}