#include "ppos.h"
#include <stdlib.h>
#include <stdio.h>

int tid;
task_t *curTask = NULL, prevTask, mainTask;


void ppos_init (){
    tid = 0; // Não será usado por tarefas criadas

    mainTask.prev = NULL;
    mainTask.next = NULL;
    mainTask.id = 0;
    getcontext(&(mainTask.context));

    curTask = &mainTask;

    setvbuf (stdout, 0, _IONBF, 0) ;
}

/*
 * Inicializa task com ponteiros de fila iguais a NULL e com um 
 * id único
 * Retornos de erro
 *  -1: Ponteiro de task aponta para NULL
 *  -2: Erro na alocação da pilha
 */
int task_create (task_t *task, void (*start_func)(void *), void *arg){
    char *stack;
    
    if (!task)
        return -1;

    stack = malloc(STACKSIZE);
    if (!stack)
        return -2;

    getcontext(&(task->context));
    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;
    makecontext(&(task->context), (void *) start_func, 1, (void *) arg);

    task->next = NULL;
    task->prev = NULL;
    task->id = ++tid;
    return tid;
}

/*
 * Retornos de erro
 *  -1: Task passada como parâmetro é NULL
 */
int task_switch(task_t *task){
    if (!task)
        return -1;
    
    task_t *taskPtr = curTask;
    curTask = task;
    swapcontext(&(taskPtr->context), &(task->context));
    return 0;
}

void task_exit (int exit_code) {
    ucontext_t auxContext;

    curTask = &mainTask;
    swapcontext(&auxContext, &(mainTask.context));
}

int task_id() {
    if (!curTask)
        return 0;
    return curTask->id;
}
