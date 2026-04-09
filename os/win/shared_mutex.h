#ifndef SHARED_MUTEX_H
#define SHARED_MUTEX_H


void init_process_mutex();
void destroy_process_mutex();
void lock_process_mutex();
void unlock_process_mutex();

#endif // SHARED_MUTEX_H
