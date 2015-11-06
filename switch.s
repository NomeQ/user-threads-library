// CS 533 Project
// Naomi Dickerson

/* Routine for switching thread contexts */

# void thread_switch(Thread * old, Thread * new)

/*
%rdi = old TCB
%rsi = new TCB
%rsp = stack pointer
%rbp = frame pointer
%r12-r15 = miscellaneous
old thread stack ptr = (%rdi)
new thread stack ptr = (%rsi) 
*/

.globl thread_switch

thread_switch:
    pushq %rbp            #push callee-saves onto stack
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq  %rsp, (%rdi)   #save stack pointer to TCB
    movq  (%rsi), %rsp   #load stack pointer from new TCB
    popq  %r15            #pop values in reverse order
    popq  %r14
    popq  %r13
    popq  %r12
    popq  %rbx
    popq  %rbp
    ret

/* Switch thread contexts to a new thread */

# void thread_start(struct thread * old, struct thread * new);

.globl thread_start

thread_start:
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq  %rsp, (%rdi)
    movq (%rsi), %rsp
    jmp thread_wrap 
