# user-threads-library
A User-Level Threads Library

Round-robin scheduling for unlimited user threads running on multiple kernel threads. Includes wrapper for
an asynchronous 'read'. Uses spinlocks, a single ready-list and free-list. API modeled on Pthreads.
