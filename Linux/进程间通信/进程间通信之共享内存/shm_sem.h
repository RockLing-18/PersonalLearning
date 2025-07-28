#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>

#define SHM_SIZE 1024
#define SEM_KEY 0x1234
#define PATHNAME "/tmp"  // ftok 路径

// 信号量操作联合体（System V 要求）
union semun 
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// 初始化信号量
int init_sem(int semid, int val) 
{
    union semun arg;
    arg.val = val;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl");
        return -1;
    }
    return 0;
}

// P 操作（获取锁）
void P(int semid) 
{
    struct sembuf op = {0, -1, 0};  // 对信号量 0 执行 -1 操作
    if (semop(semid, &op, 1) == -1) {
        perror("semop P");
        exit(1);
    }
}

// V 操作（释放锁）
void V(int semid) 
{
    struct sembuf op = {0, 1, 0};  // 对信号量 0 执行 +1 操作
    if (semop(semid, &op, 1) == -1) {
        perror("semop V");
        exit(1);
    }
}