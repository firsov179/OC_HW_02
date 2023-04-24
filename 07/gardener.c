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


void start_gardener(int count, char* name) { // функция эмулирующая работу садовника
    for (int day = 1;; ++day) {
        int cur_count = count;
        for (int i = 0; cur_count > 0 && i < FLOWERS; i++) {
            sem_wait(sem_id); // чтобы 2 садовника не полили одновременно устанавливаем семафор
            if (flowerbed->flowers[i] == NEED_WATER) { // если цветок засох, то поливает его
                printf("Садовник %s поливает цветок c номером %d на %ld-м дне.\n", name, i, day);
                flowerbed->flowers[i] = NORMAL;
                cur_count--; // уменьшаем колличество воды (количество цветков которые он может еще полить)
            }
            // Отпускаем семафор
            sem_post(sem_id);
        }
        sleep(TIME_SLEEP);
    }
}

int main(int argc, char *argv[]) {
    printf("aaa\n");

    signal(SIGINT, sign);
    signal(SIGTERM, sign);
    srand(time(NULL));

    {
        sem_id = sem_open(SEM_NAME, O_CREAT, 0666, 0);
        if (sem_id == 0) {
            perror("sem_open");
            exit(EXIT_FAILURE);
        }
    }

    // Работа с памятью
    {
        // Создаем разделяемую память
        buf_id = shm_open(SHM_NAME, O_RDWR, 0666);
        if (buf_id == -1) {
            perror("shm_open error");
            exit(EXIT_FAILURE);
        }

        // Подключение к разделяемой памяти
        flowerbed = mmap(NULL, sizeof(Flowerbed), PROT_READ | PROT_WRITE, MAP_SHARED, buf_id, 0);
        if (flowerbed == (Flowerbed *) -1) {
            perror("mmap garden");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < FLOWERS; i++) {
        flowerbed->flowers[i] = NORMAL;
    }
    printf("bbb\n");
    switch(fork()) {
        case -1:
            perror("fork"); // произошла ошибка
            exit(1); // выход из родительского процесса
        case 0:
            start_gardener(4, "Vasilii");
            return 0;
        default:
            start_gardener(5, "Petr");
            return 0;
    }
}