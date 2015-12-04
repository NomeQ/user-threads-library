/* 
CS 533 Class Project
Naomi Dickeron

Scheduler for a user-level threads package. 
*/
#define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "scheduler.h"
#include "async.h"

#define STACK_SIZE 1024 * 1024

// Global Variables
struct queue ready_list;
struct queue free_list;

// Prototype for assembly code to switch between two existing threads
void thread_switch(struct thread * old, struct thread * new);
// Prototype for assembly code to switch to a new thread
void thread_start(struct thread * old, struct thread * new);

// Called at the completion of the thread_wrap() wrapper
// Notify any threads waiting for us to finish from thread_join
void thread_finish() {
    mutex_lock(&current_thread->thread_lock);
    current_thread->state = DONE;
    condition_broadcast(&current_thread->thread_finished);
    mutex_unlock(&current_thread->thread_lock);
    yield();
}

// Calls the initial function of a thread with the initial argument, then
// calls thread_finish() to prevent popping the stack and getting a 
// memory error
void thread_wrap() {
    current_thread->initial_function(current_thread->initial_argument);
    thread_finish();
}

// Exported API functions for scheduler

// Initializes the ready list, free list, and current thread
void scheduler_begin() {
    BYTE * kernel_stack;
    int flgs, tmp;
    tmp = 1;
    flgs = CLONE_THREAD | CLONE_VM | CLONE_SIGHAND | CLONE_FILES | CLONE_FS | CLONE_IO;
    kernel_stack = malloc(STACK_SIZE) + STACK_SIZE; 
    printf("About to start kernel thread...");
    clone(kernel_thread_begin, kernel_stack, flgs, &tmp);
    ready_list.head = NULL; 
    ready_list.tail = NULL;
    free_list.head = NULL;
    free_list.tail = NULL;
    
}

int kernel_thread_begin(void * trash) {
    set_current_thread((struct thread*) malloc(sizeof(struct thread)));
    current_thread->state = RUNNING;
    while (1) {
        yield();
    }
    return 0;
}

// Create a new thread and begin running it
struct thread * thread_fork(void(*target)(void*), void * arg) {
    struct thread * t;

/* Currently, reclaiming threads from the free list does not work
with 'Join'.  
    // If there are any finished threads available, reclaim one
    if (!is_empty(&free_list)) {
        //printf("reusing old thread...");
        t = thread_dequeue(&free_list);
        // reset its stack pointer to the initial position
        t->stack_pointer = t->stack_init + STACK_SIZE;
    // Otherwise, allocate new memory for a new thread
    } else {
        t = (struct thread*) malloc(sizeof(struct thread));
        t->stack_pointer = malloc(STACK_SIZE) + STACK_SIZE;
        t->stack_init = t->stack_pointer - STACK_SIZE;
    }
*/
    // Initialize all fields of the TCB
    t = (struct thread *) malloc(sizeof(struct thread));
    t->stack_pointer = malloc(STACK_SIZE) + STACK_SIZE;
    t->stack_init = t->stack_pointer - STACK_SIZE;
    t->initial_function = target;
    t->initial_argument = arg;
    t->state = RUNNING;
    mutex_init(&t->thread_lock);
    condition_init(&t->thread_finished);

    // Swap the newly forked thread with the current one
    current_thread->state = READY;
    thread_enqueue(&ready_list, current_thread);
    struct thread * temp = current_thread;
    set_current_thread(t);
    thread_start(temp, current_thread);
    return t;  
}

// Wait until a given thread has terminated to continue
void thread_join(struct thread * th) {
    // Okay, with our current cooperative method of threading, we
    // don't actually have to worry about a race condition on
    // thread->state yet, but using a mutex anyway for good condition
    // variable semantics.
    mutex_lock(&th->thread_lock);
    // Once the status has been changed to DONE, it will stay at 
    // DONE, so no need to recheck the state once we're signaled
    if (th->state != DONE) {
        condition_wait(&th->thread_finished, &th->thread_lock);
    }
    mutex_unlock(&th->thread_lock);
}

// Voluntarily yield the CPU. If the thread is DONE, place it
// on the free_list, if it is blocked, don't enqueue it on any list
void yield() {
    switch(current_thread->state) {
        case DONE :
            thread_enqueue(&free_list, current_thread);
            break;
        case BLOCKED :
            if (is_empty(&ready_list)) {
                printf("ERROR: thread blocking, no threads to run\n");
                exit(1);
            }
            break;
        default :
            current_thread->state = READY;
            thread_enqueue(&ready_list, current_thread);
    }
    struct thread * temp = current_thread;
    set_current_thread(thread_dequeue(&ready_list));
    current_thread->state = RUNNING;
    thread_switch(temp, current_thread);
}

// Wait until all threads finish before freeing memory and
// allowing the main thread to finish and terminate the program
void scheduler_end() {
    while (!is_empty(&ready_list)) {
        yield();
    }
    while (!is_empty(&free_list)) {
        struct thread * temp = thread_dequeue(&free_list);
        free(temp->stack_init);
        free(temp);
    }
    free(current_thread);
}

// Synchronization

// Mutex
// Blocking mutex for use with cooperative threading
void mutex_init(struct mutex * m) {
    // mutex is not held
    m->held = 0;
    m->waiting_threads.head = NULL;
    m->waiting_threads.tail = NULL;
}

// If the mutex is already held, place the thread on the 
// waiting list, block, and yield. Otherwise, set held to true.
void mutex_lock(struct mutex * m) {
    if (m->held == 1) {
        current_thread->state = BLOCKED;
        thread_enqueue(&m->waiting_threads, current_thread);
        yield();
    }
    m->held = 1;
}

// If there is a thread waiting to acquire the mutex, take it
// off the waiting list, set it to READY, and put it back on 
// the ready list; do not reset held, so no other threads can get
// the mutex in the meantime. Otherwise, release the mutex.
void mutex_unlock(struct mutex * m) {
    if (!is_empty(&m->waiting_threads)) {
        struct thread * temp = thread_dequeue(&m->waiting_threads);
        temp->state = READY;
        thread_enqueue(&ready_list, temp);
    } else {
        m->held = 0;
    }
}

// Blocking Condition variables for cooperative threading
void condition_init(struct condition * c) {
    c->waiting_threads.head = NULL;
    c->waiting_threads.tail = NULL;
}

// Release the given mutex, queue up on the condition's waiting list,
// set status to BLOCKED, and yield, transferring control away until
// a call to signal/broadcast puts us back on the ready list. Re-acquire
// the mutex upon reentry
void condition_wait(struct condition * c, struct mutex * m) {
    mutex_unlock(m);
    thread_enqueue(&c->waiting_threads, current_thread);
    current_thread->state = BLOCKED;
    yield();
    mutex_lock(m);
}  

// If a thread is waiting on this condition, place it back on the 
// ready list
void condition_signal(struct condition * c) {
    if (!is_empty(&c->waiting_threads)) {
        struct thread * temp = thread_dequeue(&c->waiting_threads);
        temp->state = READY;
        thread_enqueue(&ready_list, temp);
    }
}  

// Place all (if any) waiting threads back on the ready list
void condition_broadcast(struct condition * c) {
    while (!is_empty(&c->waiting_threads)) {
        struct thread * temp = thread_dequeue(&c->waiting_threads);
        temp->state = READY;
        thread_enqueue(&ready_list, temp);
    }
}

// Some print methods for convenient debugging/status outputs

void print_queue(struct queue_node * node) {
    if (node != NULL) {
        struct thread * t = node->t;
        printf("Thread %p : State = %d\n", t, t->state);
        print_queue(node->next);
    }
}

void print_queues(struct queue_node * node) {
    if (node != NULL) {
        struct thread * t = node->t;
        printf("\nThread at address %p has state %d\n", t, t->state);
        printf("Threads waiting on this thread to terminate:\n");
        print_queue(t->thread_finished.waiting_threads.head);
        printf("\n");
        print_queues(node->next);
    }
}

void print_readylist() {
    printf("\nPrinting ready_list...\n");
    print_queues(ready_list.head);    
}

void print_freelist() {
    printf("\nPrinting free_list...\n");
    print_queues(free_list.head);
}
