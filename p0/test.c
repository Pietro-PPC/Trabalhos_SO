#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

typedef struct intqueue_s{
    struct intqueue_s *prev ;
    struct intqueue_s *next ;
    int value;
} intqueue_t;

void print_elem(void *ptr){
    intqueue_t *elem = ptr;
    printf("%d", elem->value);
}

int main(){
    intqueue_t *queue = NULL;
    intqueue_t *item1, *item2, *item3;
    item1 = malloc(sizeof(intqueue_t));
    item2 = malloc(sizeof(intqueue_t));
    item3 = malloc(sizeof(intqueue_t));
    item1->value = 1;
    item2->value = 2;
    item3->value = 3;

    queue_append((queue_t **) &queue, (queue_t *)item1);
    queue_append((queue_t **) &queue, (queue_t *)item2);
    queue_append((queue_t **) &queue, (queue_t *)item3);

    queue_print("Filarmonica", (queue_t *) queue, print_elem);
    fflush(stdout);
    while(queue_size((queue_t *) queue)){
        printf("%d\n", queue_remove((queue_t **) &queue, (queue_t *)queue));
        fflush(stdout);
        queue_print("Filarmonica", (queue_t *) queue, print_elem);
    }

    printf("\n");
    free(item1);
    free(item2);
    free(item3);
    return 0;
}
