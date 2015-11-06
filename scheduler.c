/* 
CS 533 Class Project
Naomi Dickeron

Scheduler for a user-level threads package. 
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "scheduler.h"
#include "queue.h"
#include "async.h"

#define STACK_SIZE 1024 * 1024

// Global Variables
struct thread * current_thread;
struct queue ready_list;
struct queue free_list;

// Prototype for assembly code to switch between two existing threads
void thread_switch(struct thread * old, struct thread * new);
// Prototype for assembly code to switch to a new thread
void thread_start(struct thread * old, struct thread * new);

// Called at the completion of the thread_wrap() wrapper
void thread_finish() {
    current_thread->state = DONE;
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
    current_thread = (struct thread*) malloc(sizeof(struct thread));
    current_thread->state = RUNNING;
    ready_list.head = NULL; 
    ready_list.tail = NULL;
    free_list.head = NULL;
    free_list.tail = NULL;
}

// Create a new thread and begin running it
void thread_fork(void(*target)(void*), void * arg) {
    struct thread * t;
    // If there are any finished threads available, reclaim one
    if (!is_empty(&free_list)) {
        //printf("reusing old thread...");
        t = thread_dequeue(&free_list);
        // clear old thread's stack and reset the stack ptr
        memset(t->stack_init, 0, STACK_SIZE);
        t->stack_pointer = t->stack_init + STACK_SIZE;
    // Otherwise, allocate new memory for a new thread
    } else {
        t = (struct thread*) malloc(sizeof(struct thread));
        t->stack_pointer = malloc(STACK_SIZE) + STACK_SIZE;
        t->stack_init = t->stack_pointer - STACK_SIZE;
    }
    t->initial_function = target;
    t->initial_argument = arg;
    t->state = RUNNING;
    current_thread->state = READY;
    thread_enqueue(&ready_list, current_thread);
    struct thread * temp = current_thread;
    current_thread = t;
    thread_start(temp, current_thread);  
}

// Voluntarily yield the CPU. If the thread is DONE, place it
// on the free_list
void yield() {
    if (current_thread->state != DONE) {
        current_thread->state = READY;
        thread_enqueue(&ready_list, current_thread);
    } else {
        thread_enqueue(&free_list, current_thread);
    }
    struct thread * temp = current_thread;
    current_thread = thread_dequeue(&ready_list);
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
