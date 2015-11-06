
typedef unsigned char BYTE;

typedef enum {
    RUNNING, // The thread is currently running.
    READY,   // The thread is not running, but is runnable.
    BLOCKED, // The thread is not running, and not runnable.
    DONE     // The thread has finished.
} state_t;

struct thread {
    BYTE * stack_pointer;
    void (*initial_function)(void*);
    void * initial_argument;
    BYTE * stack_init;
    state_t state;
};

void scheduler_begin();

void thread_fork(void(*target)(void*), void * arg);

void yield();

void scheduler_end();

extern struct thread * current_thread;
