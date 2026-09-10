#include <stdlib.h>
#include <stdio.h>
#include "../include/threadpool.h"

static void *worker_thread( void *arg ){
	threadpool_t *pool = ( threadpool_t * ) arg;

	while( 1 ){
		thread_task_t task;

		pthread_mutex_lock( &pool->lock );

		while( pool->count == 0 && !pool->shutdown ){
			pthread_cond_wait( &pool->not_empty, &pool->lock );
		}

		if( pool->shutdown && pool->count == 0 ){
			pthread_mutex_unlock( &pool->lock );
			break;
		}

		task = pool->queue[ pool->head ];
		pool->head = ( pool->head + 1 ) % QUEUE_SIZE;
		pool->count--;

		pthread_cond_signal( &pool->not_full );
		pthread_mutex_unlock( &pool->lock );

		if( task.function != NULL ){
			task.function( task.client_socket );
		}
	}

	return NULL;
}

void threadpool_init( threadpool_t *pool, int thread_count ){
	pool->head = 0;
	pool->tail = 0;
	pool->count = 0;
	pool->shutdown = false;
	pool->thread_count = thread_count;
	
	pthread_mutex_init( &pool->lock, NULL );
	pthread_cond_init( &pool->not_empty, NULL );
	pthread_cond_init( &pool->not_full, NULL );

	pool->threads = malloc( sizeof( pthread_t ) * thread_count );
	if( pool->threads == NULL ){
		perror( "threadpool malloc" );
		exit( EXIT_FAILURE );
	}

	for( int i = 0; i < thread_count; i++ ){
		pthread_create( &pool->threads[ i ], NULL, worker_thread, pool );
	}
}

void threadpool_submit( threadpool_t *pool, task_func_t function, int client_socket ){
	pthread_mutex_lock( &pool->lock );

	while( pool->count == QUEUE_SIZE && !pool->shutdown ){
		pthread_cond_wait( &pool->not_full, &pool->lock );
	}

	if( pool->shutdown ){
		pthread_mutex_unlock( &pool->lock );
		return;
	}

	pool->queue[ pool->tail ].function = function;
	pool->queue[ pool->tail ].client_socket = client_socket;
	pool->tail = ( pool->tail + 1 ) % QUEUE_SIZE;
	pool->count++;

	pthread_cond_signal( &pool->not_empty );
	pthread_mutex_unlock( &pool->lock );
}

void threadpool_destroy( threadpool_t *pool ){
	pthread_mutex_lock( &pool->lock );
	pool->shutdown = true;
	pthread_cond_broadcast( &pool->not_empty );
	pthread_mutex_unlock( &pool->lock );

	for( int i = 0; i < pool->thread_count; i++ ){
		pthread_join( pool->threads[ i ], NULL );
	}

	free( pool->threads );
	pthread_mutex_destroy( &pool->lock );
	pthread_cond_destroy( &pool->not_empty );
	pthread_cond_destroy( &pool->not_full );
}
