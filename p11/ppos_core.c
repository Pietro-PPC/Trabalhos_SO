// GRR20190430 Pietro Polinari Cavassin

#include "ppos.h"
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>

void dispatcher();
void print_task(void *_task);
void set_default_values_task(task_t *task);
void treat_quantum();

int totalTicks, lock = 0;
struct sigaction quantum_int;
struct itimerval quantum_timer;
int tid, userTasks = 0, ticksLeft;
task_t *curTask = NULL, mainTask, dispatcherTask, *readyTasks = NULL, *sleepingTasks = NULL;

unsigned int systime(){
    return totalTicks;
}

void ppos_init ()
{
    tid = 0; // ID 0 usado apenas na main

    // Cria task da main e a insere na fila de prontas
    set_default_values_task(&mainTask);
    mainTask.id = 0;
    mainTask.status = RODANDO;
    mainTask.preemptable = 1;
    getcontext(&(mainTask.context));
    curTask = &mainTask;

    mainTask.preemptable = 1;
    userTasks++;
    queue_append((queue_t **) &readyTasks, (queue_t *) &mainTask);

    // Cria task do dispatcher
    if (task_create(&dispatcherTask, dispatcher, NULL) == -2)
    {
        perror("Erro na alocação da pilha para o dispatcher!\n");
        exit(1);
    };

    // Cria sigaction para interrupção de quantum
    quantum_int.sa_handler = treat_quantum;
    sigemptyset (&quantum_int.sa_mask);
    quantum_int.sa_flags = 0;
    if (sigaction (SIGALRM, &quantum_int, 0) < 0)
    {
        perror ("Erro na criação da sigaction de quantum") ;
        exit (1) ;
    }

    // Cria timer de quantum
    quantum_timer.it_value.tv_usec = QUANTUM_MICRO ;
    quantum_timer.it_value.tv_sec = QUANTUM_SEC ;
    quantum_timer.it_interval.tv_usec = QUANTUM_MICRO ;
    quantum_timer.it_interval.tv_sec = QUANTUM_SEC ;
    if (setitimer (ITIMER_REAL, &quantum_timer, 0) < 0)
    {
        perror ("Erro na criação de timer de quantum") ;
        exit (1) ;
    }
    // Inicializa quantidade de ticks 
    ticksLeft = TASK_TICKS;
    totalTicks = 0;

    setvbuf (stdout, 0, _IONBF, 0) ;
}

/*
 * Inicializa task com ponteiros de fila iguais a NULL e com um 
 * id único
 * Retornos de erro
 *  -1: Ponteiro de task aponta para NULL
 *  -2: Erro na alocação da pilha
 */
int task_create (task_t *task, void (*start_func)(void *), void *arg)
{
    char *stack;
    
    if (!task)
        return -1;

    stack = malloc(STACKSIZE);
    if (!stack)
        return -2;
    
    set_default_values_task(task);
    task->id = ++tid;
    task->creation_time = totalTicks;

    getcontext(&(task->context));
    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;
    makecontext(&(task->context), (void *) start_func, 1, (void *) arg);


    #ifdef DEBUG
    printf("Task %d criada por Task %d\n", task->id, curTask->id);
    #endif
    if (tid == 1) return tid;

    task->preemptable = 1;
    userTasks++;
    queue_append((queue_t **) &readyTasks, (queue_t *) task);
    #ifdef DEBUG
    queue_print("Fila de prontas", (queue_t *) readyTasks, (void *) print_task);
    #endif

    return tid;
}

/*
 * Retornos de erro
 *  -1: Task passada como parâmetro é NULL
 */
int task_switch(task_t *task)
{
    if (!task)
        return -1;
    task->activations++;

    task_t *taskPtr = curTask;
    curTask = task;
    
    #ifdef DEBUG
    printf("Troca de tasks: %d -> %d\n", taskPtr->id, curTask->id);
    #endif

    swapcontext(&(taskPtr->context), &(task->context));
    return 0;
}

void task_exit (int exit_code) 
{
    task_t *prevTask = curTask;
    curTask->status = TERMINADA;
    curTask->exit_code = exit_code;
    if (curTask->id != 1)
        userTasks--;

    // Acorda todas as tarefas suspensas
    while (curTask->suspended)
        task_resume(curTask->suspended, &curTask->suspended);

    curTask = (curTask == &dispatcherTask) ? &mainTask : &dispatcherTask;

    #ifdef DEBUG
    printf("Saída de task: %d -> %d\n", prevTask->id, curTask->id);
    #endif

    printf("Task %d exit: execution time %5lld ms, processor time %5lld ms, %5lld activations\n", 
        prevTask->id, totalTicks - prevTask->creation_time, prevTask->processor_time, prevTask->activations);
    swapcontext(&(prevTask->context), &(curTask->context));
}

int task_id() 
{
    if (!curTask)
        return 0;
    return curTask->id;
}

void task_yield()
{
    curTask->status = PRONTA;
    task_switch(&dispatcherTask);
}

/*
 * Escalonador. Retorna NULL caso fila de prontos esteja vazia
 */
task_t *scheduler() 
{
    if (!readyTasks) 
        return NULL;

    // Seleciona próxima task de maior prioridade
    task_t *ret = readyTasks;
    task_t *task_ptr = readyTasks->next;
    while(task_ptr != readyTasks)
    {
        if (task_ptr->dynamic_prio < ret->dynamic_prio)
            ret = task_ptr;
        task_ptr = task_ptr->next;
    }

    // Atualiza prioridades das tasks
    task_ptr = readyTasks;
    do 
    {
        if (task_ptr->dynamic_prio > MIN_PRIO)
            (task_ptr->dynamic_prio)--;
        task_ptr = task_ptr->next;
    } while (task_ptr != readyTasks);
    
    // Prioridade da próxima tarefa a ser executada volta ao valor estático
    ret->dynamic_prio = ret->static_prio;

    return ret;
}

void wakeup_tasks()
{
    if (!sleepingTasks)
        return;
    
    task_t *curTask = sleepingTasks, *nextTask;
    do // acorda tasks que devem ser acordadas
    {
        nextTask = curTask->next;
        if (totalTicks >= curTask->wakeup_time)
            task_resume(curTask, &sleepingTasks);
        curTask = nextTask;
    } while (nextTask != sleepingTasks && sleepingTasks );
}

void dispatcher() 
{
    while (userTasks || sleepingTasks)
    {
        wakeup_tasks();
        task_t *nextTask = scheduler();
        if (nextTask)
        {
            #ifdef DEBUG
            queue_print("Fila de prontas", (queue_t *) readyTasks, (void *) print_task);
            #endif
            // Tarefa sai da fila de prontas, pois estará rodando
            ticksLeft = TASK_TICKS;
            queue_remove((queue_t **) &readyTasks, (queue_t *) nextTask);
            nextTask->status = RODANDO;
            task_switch(nextTask);

            switch(nextTask->status)
            {
                case PRONTA:
                    queue_append((queue_t **) &readyTasks, (queue_t *) nextTask);
                    break;
                case TERMINADA:
                    free(nextTask->context.uc_stack.ss_sp);
                    break;
            }
        }
    }
    task_exit(0);
}

/* 
 * Seta a prioridade estática da tarefa task para prio.
 * Se task == NULL, seta a prioridade da tarefa atual
 */
void task_setprio (task_t *task, int prio)
{
    if (!task)
        task = curTask;

    task->static_prio = prio;
    task->dynamic_prio = prio;
}

/*
 * Retorna prioridade estática da tarefa 
 */
int task_getprio (task_t *task)
{
    if (!task)
        task = curTask;
    return task->static_prio;
}

/*
 * Tratador chamado ao fim de um quantum
 */
void treat_quantum()
{
    totalTicks++;
    ticksLeft--;
    curTask->processor_time++;
    if (curTask->preemptable && ticksLeft == 0)
        task_yield();
}

/**
 * Suspende tarefa atual, adiciona em queue e volta para o dispatcher
 */
void task_suspend(task_t **queue)
{
    // Se está executando, já está na fila de prontas. Mas retiro por boa prática.
    queue_remove((queue_t **) &readyTasks, (queue_t *) curTask);
    curTask->status = SUSPENSA;
    queue_append((queue_t **) queue, (queue_t *) curTask);
    task_yield();
}

/**
 * Retira tarefa task de queue e a insere na fila de prontas.
 */
void task_resume (task_t *task, task_t **queue) 
{
    queue_remove ((queue_t **) queue, (queue_t *) task);
    task->status = PRONTA;
    queue_append ((queue_t **) &readyTasks, (queue_t *) task);
}

/**
 * Suspende a tarefa atual até o fim da task task
 * Retornos:
 *  . -1: A tarefa task não existe ou já foi encerrada
 */
int task_join(task_t *task)
{
    if (!task)
        return -1;
    if (task->status == TERMINADA)
        return -1;

    task_suspend(&(task->suspended));
    return task->exit_code;
}

void task_sleep(int t)
{
    queue_remove((queue_t **) &readyTasks, (queue_t *) curTask);
    curTask->wakeup_time = totalTicks + t;
    
    queue_append((queue_t **) &sleepingTasks, (queue_t *) curTask);
    task_yield();
}

void enter_cs (int *lock)
{
  // atomic OR (Intel macro for GCC)
  while (__sync_fetch_and_or (lock, 1)) ;   // busy waiting
}

void leave_cs (int *lock)
{
  (*lock) = 0 ;
}

int sem_create (semaphore_t *s, int value) {
    if (!s)
        return -1;

    s->count = value;
    s->q = NULL;
    return 0;
}

// requisita o semáforo
int sem_down (semaphore_t *s) {
    if (!s)
        return -1;
    enter_cs(&lock);
    s->count--;
    if (s->count < 0){
        queue_append((queue_t **) &(s->q), (queue_t *) curTask);
        curTask->status = SUSPENSA;
    }
    leave_cs(&lock);
    if (curTask->status == SUSPENSA)
        task_yield();

    return 0;
}

// libera o semáforo
int sem_up (semaphore_t *s) {

    if (!s)
        return -1;

    enter_cs(&lock);
    s->count++;
    if (s->count <= 0){
        task_resume(s->q, &(s->q));
    }
    leave_cs(&lock);

    return 0;
}

// destroi o semáforo, liberando as tarefas bloqueadas
int sem_destroy (semaphore_t *s) {
    if (!s)
        return -1;
    
    while (sem_up(s) != -1);
    
    return 0;
}


// FUNÇÕES ADICIONAIS

/*
 * Imprime task passada como argumento. A ser usada na função queue_print
 */
void print_task(void *_task)
{
    task_t *task = _task;
    printf("%d(%d)", task->id, task->dynamic_prio);
}


/*
 * Inicializa valores default de uma task
 */
void set_default_values_task(task_t *task)
{
    task->prev = NULL;
    task->next = NULL;
    task->status = PRONTA;
    task->static_prio = 0;
    task->dynamic_prio = 0;
    task->preemptable = 0; // por segurança, tarefas não são preemptáveis por padrão

    task->processor_time = 0;
    task->activations = 0;
    task->suspended = NULL;
}
