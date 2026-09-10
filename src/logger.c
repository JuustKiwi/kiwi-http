#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include "../include/logger.h"

static FILE *log_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void logger_init( const char *filepath ){
	pthread_mutex_lock( &log_mutex );
	
	log_file = fopen( filepath, "a" );
	if( log_file == NULL ){
		fprintf( stderr, "kiwi-http: Could not open log file '%s'. Falling back to stdout.\n", filepath );
		log_file = stdout;
	}
	
	pthread_mutex_unlock( &log_mutex );
}

void logger_log( const char *format, ... ){
	char time_buf[ 64 ];
	time_t now;
	struct tm *tm_info;
	va_list args;

	time( &now );
	tm_info = localtime( &now );
	strftime( time_buf, sizeof( time_buf ), "%Y-%m-%d %H:%M:%S", tm_info );

	pthread_mutex_lock( &log_mutex );
	
	if( log_file != NULL ){
		fprintf( log_file, "[%s] ", time_buf );
		
		va_start( args, format );
		vfprintf( log_file, format, args );
		va_end( args );
		
		fprintf( log_file, "\n" );
		fflush( log_file );
	}
	
	pthread_mutex_unlock( &log_mutex );
}

void logger_destroy( void ){
	pthread_mutex_lock( &log_mutex );
	
	if( log_file != NULL && log_file != stdout ){
		fclose( log_file );
		log_file = NULL;
	}
	
	pthread_mutex_unlock( &log_mutex );
}
