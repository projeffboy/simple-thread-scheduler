/* COMMENTS
 * Whenever I free a context after exit, there's an error. So I am freeing everything at shutdown instead. The former way is better but whatever.
 * Assume all task functions return nothing and have no parameters
 * Assume shutdown only happens when sut_shutdown() is called
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include "sut.h"
#include "queue.h"

int two_c_exec_threads = true; // 1 OR 2 C-EXECs!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
int task_id = 0;
int tasks_done;

pthread_t c_exec_threads[2];
pthread_t i_exec_thread;
// the main user level threads
ucontext_t c_exec_contexts[2];
ucontext_t i_exec_context;

struct queue rdy_q, wait_q;

threaddesc tasks[MAX_THREADS];
threaddesc* c_exec_tasks[2];
threaddesc* i_exec_task;

bool shutdown = false;
bool c_exec_exited[2]; // exit from C-EXECs then I-EXEC

pthread_mutex_t rdy_q_lock; // e.g. I-EXEC and C-EXEC adding to ready queue
pthread_mutex_t wait_q_lock; // e.g. two C-EXECs adding to wait queue

// HELPER FUNCTIONS

int nth_c_exec() { // only pass in an c_exec thread
    if (two_c_exec_threads == false) {
        return 0;
    } else {
        return pthread_self() == c_exec_threads[1]; // nice hack
    }
}

void put_in_wait_q() {
    int nth = nth_c_exec();

    pthread_mutex_lock(&wait_q_lock);
    struct queue_entry* node = queue_new_node(c_exec_tasks[nth]);
    queue_insert_tail(&wait_q, node);
    pthread_mutex_unlock(&wait_q_lock);
    // Go back to C-Exec
    swapcontext(&((*c_exec_tasks[nth]).threadcontext), &c_exec_contexts[nth]);
}

void put_in_rdy_q() {
    struct queue_entry* node = queue_new_node(i_exec_task);
    pthread_mutex_lock(&rdy_q_lock);
    queue_insert_tail(&rdy_q, node);
    pthread_mutex_unlock(&rdy_q_lock);
    // Go back to C-Exec
    swapcontext(&((*i_exec_task).threadcontext), &i_exec_context);
}

// THE TWO QUEUES

void* c_exec() {
    while (true) {
        pthread_mutex_lock(&rdy_q_lock);
        struct queue_entry* head = queue_peek_front(&rdy_q);

        if (head == NULL) { // quit or check again
            pthread_mutex_unlock(&rdy_q_lock);

            if (tasks_done == task_id && shutdown == true) {
                int nth = nth_c_exec();
                c_exec_exited[nth] = true;
                pthread_exit(0);
            }

            usleep(100);
        } else { // run popped task
            head = queue_pop_head(&rdy_q);
            pthread_mutex_unlock(&rdy_q_lock);

            // for Exec to keep the task globally
            threaddesc* task = head->data;
            int nth = nth_c_exec();
            c_exec_tasks[nth] = task;
            swapcontext(&c_exec_contexts[nth], &(task->threadcontext));
        }
    }
}

void* i_exec() {
    while (true) {
        pthread_mutex_lock(&wait_q_lock);
        struct queue_entry* head = queue_peek_front(&wait_q);

        if (head == NULL) { // quit or check again
            pthread_mutex_unlock(&wait_q_lock);
            if (
                c_exec_exited[0] == true
                && (two_c_exec_threads == false || c_exec_exited[1] == true)
                ) {
                pthread_exit(0);
            }

            usleep(100);
        } else { // run popped task
            head = queue_pop_head(&wait_q);
            pthread_mutex_unlock(&wait_q_lock);

            // for Exec to keep the task globally
            threaddesc* task = head->data;
            i_exec_task = task;
            swapcontext(&i_exec_context, &(task->threadcontext));
        }
    }
}

// ALL OF THE FUNCTIONS

void sut_init() { // Set up mutexes, queues, and pthreads
    c_exec_exited[0] = false;
    c_exec_exited[1] = false;

    pthread_mutex_init(&rdy_q_lock, NULL);
    pthread_mutex_init(&wait_q_lock, NULL);

    rdy_q = queue_create();
    queue_init(&rdy_q);
    wait_q = queue_create();
    queue_init(&wait_q);

    if (
        pthread_create(&i_exec_thread, NULL, i_exec, NULL) != 0
        || pthread_create(&c_exec_threads[0], NULL, c_exec, NULL) != 0
        || (
            two_c_exec_threads
            && pthread_create(&c_exec_threads[1], NULL, c_exec, NULL) != 0
            )
        ) {
        printf("Thread(s) failed to create, exiting.");
        exit(EXIT_FAILURE);
    }
}

bool sut_create(sut_task_f fn) { // creates task
    if (task_id > MAX_THREADS) {
        return false;
    }

    // set attributes
    tasks[task_id].threadid = task_id;
    tasks[task_id].threadfunc = fn;
    tasks[task_id].fd = -1;
    // set context
    if (getcontext(&tasks[task_id].threadcontext) == -1) {
        return false;
    }
    tasks[task_id].threadcontext.uc_stack.ss_sp = (char*)malloc(
        THREAD_STACK_SIZE
    );
    tasks[task_id].threadcontext.uc_stack.ss_size = THREAD_STACK_SIZE;
    tasks[task_id].threadcontext.uc_link = 0;
    makecontext(&tasks[task_id].threadcontext, fn, 0);

    tasks[task_id].mem_alloc = true;

    // add task to ready queue
    struct queue_entry* new_node = queue_new_node(&tasks[task_id]);
    task_id++;
    pthread_mutex_lock(&rdy_q_lock);
    queue_insert_tail(&rdy_q, new_node);
    pthread_mutex_unlock(&rdy_q_lock);

    return true;
}

void sut_yield() { // put task back in queue, pause task
    int nth = nth_c_exec();
    struct queue_entry* new_node = queue_new_node(c_exec_tasks[nth]);
    pthread_mutex_lock(&rdy_q_lock);
    queue_insert_tail(&rdy_q, new_node);
    pthread_mutex_unlock(&rdy_q_lock);

    swapcontext(&((*c_exec_tasks[nth]).threadcontext), &c_exec_contexts[nth]);
}

void sut_exit() { // free memory and exit thread
    // Doesn't work if I free here so I'm just going to free at shutdown
    // char* threadstack = (*c_exec_task[0]).threadcontext.uc_stack.ss_sp;
    // free(threadstack);

    int nth = nth_c_exec();
    (*c_exec_tasks[nth]).mem_alloc = false; // not necessary if not freeing at sut_exit

    tasks_done++;
    setcontext(&c_exec_contexts[nth]);
}

void sut_shutdown() { // join pthreads and free anything remaining
    shutdown = true;
    if (
        pthread_join(c_exec_threads[0], NULL) != 0
        || pthread_join(i_exec_thread, NULL) != 0
        || (
            two_c_exec_threads
            && pthread_create(&c_exec_threads[1], NULL, c_exec, NULL) != 0
            )
        ) {
        printf("Failed to join thread(s), exiting.");
        exit(EXIT_FAILURE);
    };

    for (int i = 0; i < tasks_done; i++) {
        if (tasks[i].mem_alloc == false) {
            free(tasks[i].threadcontext.uc_stack.ss_sp);
        }
    }
}

// I-EXEC RELATED FUNCTIONS

int sut_open(char* fname) {
    // From C-Exec
    put_in_wait_q();

    // I-Exec resumes
    int fd = open(fname, O_RDWR); // could even be -1
    (*i_exec_task).fd = fd;

    put_in_rdy_q();

    // C-Exec resumes
    return fd;
}

void sut_close(int fd) {
    // From C-Exec
    put_in_wait_q();

    // I-Exec resumes
    close(fd);
    (*i_exec_task).fd = -1;

    put_in_rdy_q();

    // C-Exec resumes
}

char* sut_read(int fd, char* buf, int size) {
    // From C-Exec
    put_in_wait_q();

    // I-Exec resumes
    int num_bytes = read(fd, buf, size * sizeof(char));

    put_in_rdy_q();

    // C-Exec resumes
    if (num_bytes == -1) {
        return NULL;
    } else {
        return buf;
    }
}

void sut_write(int fd, char* buf, int size) {
    // From C-Exec
    put_in_wait_q();

    // I-Exec resumes
    write(fd, buf, size * sizeof(char));

    put_in_rdy_q();

    // C-Exec resumes
}