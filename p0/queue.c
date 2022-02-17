#include <stdio.h>
#include "queue.h"

int queue_size (queue_t *queue){
    if (!queue) 
        return 0;

    queue_t *ptr = queue->next;
    int size = 1;
    while (ptr != queue){
        ptr = ptr->next;
        size++;
    }
    
    return size;
}

void queue_print (char *name, queue_t *queue, void print_elem (void*) ){
    printf("Fila %s\n", name);

    if (!queue)
        return;

    queue_t *it = queue;
    do {
        print_elem((void*) it);
        printf(" ");
        it = it->next;
    } while(it != queue);
}

/*
    -1: Elemento não existe
    -2: Elemento em outra fila
*/
int queue_append (queue_t **queue, queue_t *elem){
    if (!elem)
        return -1;

    if (elem->next || elem->prev)
        return -2;
    
    if (!(*queue)){
        elem->next = elem;
        elem->prev = elem;
        *queue = elem;
    } else {
        queue_t *last = (*queue)->prev;
        // ajusta ponteiros com ultimo elemento
        last->next = elem;
        elem->prev = last;
        // ajusta ponteiros com primeiro elemento
        elem->next = *queue;
        (*queue)->prev = elem;
    }

    return 0;
}

// -1: a fila esta vazia
// -2: o elemento não existe
// -3: o elemento não pertence a fila
// Retorno: 0 se sucesso, <0 se ocorreu algum erro
int queue_remove (queue_t **queue, queue_t *elem){
    if (!(*queue))
        return -1;

    if (!elem)
        return -2;

    // procura na fila começando do "segundo" elemento
    queue_t *it = (*queue)->next;
    while (it != elem && it != *queue)
        it++;
    
    // chegou no inicio novamente e elemento nao esta no inicio
    if (it == *queue && it != elem)
        return -3;
    
    queue_t *prev = elem->prev;
    queue_t *next = elem->next;
    prev->next = next;
    next->prev = prev;

    elem->next = NULL;
    elem->prev = NULL;

    // apenas um elemento
    if ((*queue)->next == (*queue))
        *queue = NULL;

    return 0;
}
