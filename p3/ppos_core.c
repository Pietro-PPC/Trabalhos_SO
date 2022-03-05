#include "ppos.h"
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

void dispatcher();
void print_task(void *_task);

int tid, userTasks = 0;
task_t *curTask = NULL, prevTask, mainTask, dispatcherTask, *readyTasks = NULL;

void ppos_init (){
    tid = 0; // Não será usado por tarefas criadas usando task_create

    mainTask.prev = NULL;
    mainTask.next = NULL;
    mainTask.id = 0;
    mainTask.status = RODANDO;
    getcontext(&(mainTask.context));

    task_create(&dispatcherTask, dispatcher, NULL);

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
    
    task->next = NULL;
    task->prev = NULL;
    task->id = ++tid;

    getcontext(&(task->context));
    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;
    makecontext(&(task->context), (void *) start_func, 1, (void *) arg);

    task->status = PRONTA;
    
    userTasks++;

    if (tid == 1) return tid;

    queue_append(&readyTasks, task);
    #ifdef DEBUG
    queue_print("Adicionou elemento", readyTasks, printTask);
    #endif

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
    ucontext_t *auxContext = &curTask->context;
    curTask->status = TERMINADA;
    
    curTask = (curTask == &dispatcherTask) ? &mainTask : &dispatcherTask;
    userTasks--;
    swapcontext(auxContext, &(mainTask.context));
}

int task_id() {
    if (!curTask)
        return 0;
    return curTask->id;
}

void task_yield(){
    curTask->status = PRONTA;
    task_switch(&dispatcherTask);
}

task_t *scheduler() {
    return readyTasks;
}

void dispatcher() {
    while (userTasks){ 
        task_t *nextTask = scheduler();
        if (nextTask){
            queue_remove(&readyTasks, nextTask);
            nextTask->status = RODANDO;
            task_switch(nextTask);

            switch(nextTask->status){
                case PRONTA:
                    queue_append(&readyTasks, nextTask);
                    break;
                case TERMINADA:
                    free(nextTask->context.uc_stack.ss_sp);
                    break;
            }
        }

    }
    task_exit(0);
}

// Additional functions
void print_task(void *_task){
    task_t *task = _task;
    printf("%d(%d)", task->id, task->status);
}
