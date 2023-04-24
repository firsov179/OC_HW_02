#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>


#define FLOWERS 40
#define TIME_SLEEP 2

#define SEM_NAME "/flowerbed_semaphore"// Имя семафора
#define SHM_NAME "/flowerbed"          // Имя памяти

// Состояние цветка
typedef enum { NORMAL, NEED_WATER } Status;

// Клумба
typedef struct {
    Status flowers[FLOWERS];
    int day;
} Flowerbed;

int buf_id;
Flowerbed *flowerbed;
int sem_id;
// Операция для захвата семафора
struct sembuf acquire = {0, -1, SEM_UNDO};
// Операция для освобождения семафора
struct sembuf release = {0, 1, SEM_UNDO};

void printFlowerbed(int day) { // вывод информации о клумбе
    int count_of_NORMAL = 0;
    int count_of_NEED_WATER = 0;
    printf("Цветы c номерами: [");
    for (int i = 0; i < FLOWERS; i++) {
        if (flowerbed->flowers[i] == NORMAL) {
            count_of_NORMAL++;
        } else {
            count_of_NEED_WATER++;
            printf("%d ", i); // выводим номера увядших цветов
        }
    }
    printf("] увядают на %ld-м дне.\n", day);
    printf("Свежих цветов: %d, нуждаются в поливе : %d.\n",count_of_NORMAL, count_of_NEED_WATER);
}

void sign(int sig) { // создание сигнала
    if (sig != SIGINT && sig != SIGTERM) {
        return;
    }
    semctl(sem_id, 1, IPC_RMID);
    shmctl(buf_id, IPC_RMID, NULL);

    exit(10);
}


void start_gardener(int count, char* name) { // функция эмулирующая работу садовника
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

    switch(fork()) {
        case -1:
            perror("fork"); // произошла ошибка
            exit(1); // выход из родительского процесса
        case 0:
            start_flowerbed();
            return 0;
        default:
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
}