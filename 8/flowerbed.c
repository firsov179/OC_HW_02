#include "main.c"

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>


#define FLOWERS 40
#define TIME_SLEEP 2

#define SEM_NAME "/flowerbed_semaphore"// Имя семафора
#define SHM_NAME "/flowerbed"          // Имя памяти


void sign(int sig) { // создание сигнала
    if (sig != SIGINT && sig != SIGTERM) {
        return;
    }
    semctl(sem_id, 1, IPC_RMID);
    shmctl(buf_id, IPC_RMID, NULL);

    exit(10);
}

void start_flowerbed() { // функция эмулирующая работу клумбы (увядания)
    sleep(1);
    printFlowerbed(1);
    semop(sem_id, &release, 1);
    for (int day = 1;; ++day) {
        if (day > 1) {
            semop(sem_id, &acquire, 1);
            printFlowerbed(day); // вывод информации о клумбе
        }

        flowerbed->day = day;
        for (int i = 0; i < FLOWERS; i++) {
            if (flowerbed->flowers[i] == NORMAL) {
                if (rand() % 2 == 0) {
                    flowerbed->flowers[i] = NEED_WATER;
                }
            }
        }

        semop(sem_id, &release, 1);
        sleep(TIME_SLEEP);
    }
}


int main(int argc, char *argv[]) {

    signal(SIGINT, sign);
    signal(SIGTERM, sign);
    srand(time(NULL));

    // Создаем семафор
    if ((sem_id = semget(179, 1, IPC_CREAT | 0666)) == -1) {
        perror("Ошибка при создании семафора");
        exit(1);
    }
    // Инициализируем семафор значением 1
    if (semctl(sem_id, 0, SETVAL, 1) == -1) {
        perror("Ошибка при инициализации семафора");
        exit(1);
    }
    // Создаю разделяемую память
    if ((buf_id = shmget(179, sizeof(flowerbed), IPC_CREAT | 0666)) == -1) {
        perror("Ошибка при создании разделяемой памяти");
        exit(1);
    }
    if ((flowerbed = shmat(buf_id, 0, 0)) == (Flowerbed *) -1) {
        perror("Ошибка при присоединении к разделяемой памяти");
        exit(1);
    }

    for (int i = 0; i < FLOWERS; i++) {
        flowerbed->flowers[i] = NORMAL;
    }

    start_flowerbed();
}