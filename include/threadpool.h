#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <stdbool.h>

#define QUEUE_SIZE 256

typedef void ( *task_func_t )( int client_socket );

typedef struct {
	task_func_t function;
	int client_socket;
} thread_task_t;

typedef struct {
	thread_task_t queue[ QUEUE_SIZE ];
	int head;
	int tail;
	int count;
	
	pthread_mutex_t lock;
	pthread_cond_t not_empty;
	pthread_cond_t not_full;
	
	bool shutdown;
	int thread_count;
	pthread_t *threads;
} threadpool_t;

void threadpool_init( threadpool_t *pool, int thread_count );
void threadpool_submit( threadpool_t *pool, task_func_t function, int client_socket );
void threadpool_destroy( threadpool_t *pool );

#endif
