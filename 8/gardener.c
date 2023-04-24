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


void start_gardener(int count, char *name) { // функция эмулирующая работу садовника
    for (int day = 0;; ++day) {
        int cur_count = count;
        for (int i = 0; cur_count > 0 && i < FLOWERS; i++) {
            semop(sem_id, &acquire, 1); // чтобы 2 садовника не полили одновременно устанавливаем семафор
            if (flowerbed->flowers[i] == NEED_WATER) { // если цветок засох, то поливает его
                printf("Садовник %s поливает цветок c номером %d на %ld-м дне.\n", name, i, day);
                flowerbed->flowers[i] = NORMAL;
                cur_count--; // уменьшаем колличество воды (количество цветков которые он может еще полить)
            }
            // Отпускаем семафор
            semop(sem_id, &release, 1);
        }
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

    switch (fork()) {
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