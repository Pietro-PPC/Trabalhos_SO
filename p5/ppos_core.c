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

struct sigaction quantum_int;
struct itimerval quantum_timer;
int tid, userTasks = 0, ticksLeft;
task_t *curTask = NULL, mainTask, dispatcherTask, *readyTasks = NULL;

void ppos_init ()
{
    tid = 0; // Este id não será usado por tarefas criadas usando task_create

    // Cria task da main
    set_default_values_task(&mainTask);
    mainTask.id = 0;
    mainTask.status = RODANDO;
    mainTask.preemptable = 1;
    getcontext(&(mainTask.context));
    curTask = &mainTask;

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
    if (curTask->id > 1)
        userTasks--;
    
    curTask = (curTask == &dispatcherTask) ? &mainTask : &dispatcherTask;
    
    #ifdef DEBUG
    printf("Saída de task: %d -> %d\n", prevTask->id, curTask->id);
    #endif

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
        if (task_ptr->dynamic_prio <= ret->dynamic_prio)
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

void dispatcher() 
{
    while (userTasks)
    { 
        #ifdef DEBUG
        queue_print("Fila de prontas", (queue_t *) readyTasks, (void *) print_task);
        #endif
        task_t *nextTask = scheduler();
        if (nextTask)
        {
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
void treat_quantum(){
    ticksLeft--;
    if (curTask->preemptable && ticksLeft == 0)
        task_yield();
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
}
