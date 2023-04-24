#include "main.c"

#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>


#define FLOWERS 40
#define TIME_SLEEP 2

#define SEM_NAME "/flowerbed_semaphore"// Имя семафора
#define SHM_NAME "/flowerbed"          // Имя памяти


void sign(int sig) { // создание сигнала
    if (sig != SIGINT && sig != SIGTERM) {
        return;
    }
    sem_unlink(SEM_NAME);
    sem_close(sem_id);
    shm_unlink(SHM_NAME);
    munmap(flowerbed, sizeof(Flowerbed));
    close(buf_id);

    exit(10);
}

void start_flowerbed() { // функция эмулирующая работу клумбы (увядания)
    sleep(1);
    printFlowerbed(1);
    sem_post(sem_id);
    for (int day = 1;; ++day) {
        if (day > 1) {
            sem_wait(sem_id);
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

        sem_post(sem_id);
        sleep(TIME_SLEEP);
    }
}


int main(int argc, char *argv[]) {
    signal(SIGINT, sign);
    signal(SIGTERM, sign);
    srand(time(NULL));

    if (shm_unlink(SHM_NAME) != -1) {
        sem_unlink(SEM_NAME);
        sem_close(sem_id);
        shm_unlink(SHM_NAME);
        munmap(flowerbed, sizeof(Flowerbed));
        close(buf_id);
    }
    buf_id = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (buf_id == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(buf_id, sizeof(Flowerbed)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    flowerbed = mmap(NULL, sizeof(Flowerbed), PROT_READ | PROT_WRITE, MAP_SHARED, buf_id, 0);
    if (flowerbed == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }



    if (sem_unlink(SEM_NAME) != -1) {
        perror("sem_unlink");
    }
    sem_id = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 0);
    if (sem_id == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < FLOWERS; i++) {
        flowerbed->flowers[i] = NORMAL;
    }

    start_flowerbed();
}