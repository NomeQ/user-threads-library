#include "queue.h"

#define current_thread (get_current_thread())

typedef unsigned char BYTE;

typedef enum {
    RUNNING, // The thread is currently running.
    READY,   // The thread is not running, but is runnable.
    BLOCKED, // The thread is not running, and not runnable.
    DONE     // The thread has finished.
} state_t;

struct mutex {
    int held;
    struct queue waiting_threads;
};

struct condition {
    struct queue waiting_threads;
};

struct thread {
    BYTE * stack_pointer;
    void (*initial_function)(void*);
    void * initial_argument;
    BYTE * stack_init;
    state_t state;
    struct mutex thread_lock;
    struct condition thread_finished;
};

void scheduler_begin();

struct thread * thread_fork(void(*target)(void*), void * arg);
int kernel_thread_begin(void * arg);

void yield();

void scheduler_end();

void mutex_init(struct mutex *);
void mutex_lock(struct mutex *);
void mutex_unlock(struct mutex *);

void condition_init(struct condition *);
void condition_wait(struct condition *, struct mutex *);
void condition_signal(struct condition *);
void condition_broadcast(struct condition *);

void print_readylist();
void print_freelist();

extern struct thread * get_current_thread();
extern void set_current_thread(struct thread *);
