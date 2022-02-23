int tid;
task_t *curTask;

void ppos_init (){
    tid = 0; // Não será usado por tarefas criadas

    curTask.prev = NULL;
    curTask.next = NULL;
    curTask.id = 0;
    getcontext(&(curTask.context));

    setvbuf (stdout, 0, _IONBF, 0) ;
}

/*
 * Retornos de erro
 *  -1: Erro na alocação da pilha
 */
int task_create (task_t *task, void (*start_func)(void *), void *arg){
    char *stack;
    
    stack = malloc(STACKSIZE);
    if (!stack)
        return -1;

    getcontext(&(task->context));
    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;
    makecontext(&(task->context), (void *) start_func, 1, (void *) arg);

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
    ucontext_t auxTask;
    
    curTask = task;
    swapcontext(&auxTask, &(task->context));

    return 0;
}


