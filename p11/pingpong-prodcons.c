#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ppos.h"
#include "queue.h"

#define NUM_RANGE 100
#define SLEEP_TIME 1000
#define PROD 3
#define CONS 2

typedef struct {
    queue_t *prev; 
    queue_t *next;
    int val;
} int_queue_t;

semaphore_t s_buffer, s_item, s_vaga;
int_queue_t *buffer = NULL;

void produtor(void *arg){
    int *num = arg;
    while (1){
        task_sleep(SLEEP_TIME);
        int n = rand() % NUM_RANGE;
        int_queue_t *elem = malloc(sizeof(int_queue_t));

        elem->val = n;
        elem->next = NULL;
        elem->prev = NULL;

        if (!elem) {
            fprintf(stderr, "Erro de alocação!\n");
            exit(1);
        }

        sem_down(&s_vaga);
        sem_down(&s_buffer);

        printf("p%d cria %d\n", *num, n);
        queue_append((queue_t **) &buffer, (queue_t *) elem);

        sem_up(&s_buffer);
        sem_up(&s_item);

    }
    task_exit(0);
}

void consumidor(void *arg){
    int *num = arg;
    while(1){
        sem_down(&s_item);
        sem_down(&s_buffer);
        int_queue_t *elem = buffer;
        queue_remove((queue_t **) &buffer, (queue_t *) elem);

        sem_up(&s_buffer);
        sem_up(&s_vaga);

        printf("            c%d consome %d\n", *num, elem->val);
        task_sleep(SLEEP_TIME);
    }
    task_exit(0);
}

int main(){

    ppos_init();

    srand(time(NULL));

    int nums[] = {1, 2, 3};
    printf("%d %d\n", nums[1], nums[2]);
    task_t *t ;
    sem_create(&s_buffer, 1);
    sem_create(&s_vaga, 5);
    sem_create(&s_item, 0);

    for (int i = 0; i < PROD; ++i){
        t = malloc(sizeof(task_t));
        if (!t) {
            fprintf(stderr, "Error allocating task\n");
            exit(1);
        }
        task_create(t, produtor, (void *) &nums[i]);
    }

    for (int i = 0; i < CONS; ++i){
        t = malloc(sizeof(task_t));
        if (!t) {
            fprintf(stderr, "Error allocating task\n");
            exit(1);
        }
        task_create(t, consumidor, (void *) &nums[i]);
    }

    task_sleep(20000);

    task_exit(0);

    return 0;
}