#ifndef __SUT_H__
#define __SUT_H__

#include <stdbool.h>
#include <ucontext.h>

typedef void (*sut_task_f)();

void sut_init();
bool sut_create(sut_task_f fn);
void sut_yield();
void sut_exit();
int sut_open(char* dest);
void sut_write(int fd, char* buf, int size);
void sut_close(int fd);
char* sut_read(int fd, char* buf, int size);
void sut_shutdown();

#define MAX_THREADS 32
#define THREAD_STACK_SIZE 1024 * 64

typedef struct __threaddesc {
    int threadid;
    void* threadfunc;
    ucontext_t threadcontext;
    bool mem_alloc;
    int fd; // -1 if not assigned
} threaddesc;

#endif
