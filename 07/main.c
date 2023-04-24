#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>

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
