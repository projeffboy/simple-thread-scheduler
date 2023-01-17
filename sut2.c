#include "sut2.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "queue.h"

int two_c_exec_threads = true;
int task_id = 0;
int tasks_done;

pthread_t c_exec_threads[2];
pthread_t i_exec_thread;
ucontext_t c_exec_contexts[2];
ucontext_t i_exec_context;

struct queue rdy_q, wait_q;

threaddesc tasks[MAX_THREADS];
threaddesc* c_exec_tasks[2];
threaddesc* i_exec_task;

bool shutdown = false;
bool c_exec_exited[2];

pthread_mutex_t rdy_q_lock;
pthread_mutex_t wait_q_lock;

int nth_cexec() {
  if (two_c_exec_threads == false) {
    return 0;
  } else {
    return pthread_self() == c_exec_threads[1];
  }
}

void put_in_wait_q() {
  int nth = nth_c_exec();

  pthread_mutex_lock(&wait_q_lock);
  struct queue_entry* node = queue_new_node(c_exec_tasks[nth]);
  queue_insert_tail(&wait_q_lock);
}