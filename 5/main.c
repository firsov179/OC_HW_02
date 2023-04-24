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

// Состояние цветка
typedef enum { NORMAL, NEED_WATER } Status;

// Клумба
typedef struct {
    Status flowers[FLOWERS];
    int day;
} Flowerbed;

sem_t *sem_id;
int buf_id;
Flowerbed *flowerbed;

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
    sem_destroy(sem_id);
    munmap(sem_id, sizeof(flowerbed));
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


    sem_id = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sem_id == MAP_FAILED) {
        perror("sem_id mmap");
        exit(EXIT_FAILURE);
    }
    if (sem_init(sem_id, 1, 0) == -1) {
        perror("sem_init");
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