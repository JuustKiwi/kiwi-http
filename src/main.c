#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "../include/config.h"
#include "../include/logger.h"
#include "../include/threadpool.h"
#include "../include/server.h"
#include "../include/cache.h"

threadpool_t global_pool;

void handle_signal( [[ maybe_unused ]] int sig ){
	printf( "\nkiwi-http: Shutting down ...\n" );
	server_stop();
}

int main( [[ maybe_unused ]] int argc, [[ maybe_unused ]] char **argv ){
	struct sigaction sa;

	sa.sa_handler = handle_signal;
	sigemptyset( &sa.sa_mask );
	sa.sa_flags = 0;
	sigaction( SIGINT, &sa, NULL );
	sigaction( SIGTERM, &sa, NULL );

	signal( SIGPIPE, SIG_IGN );

	if( config_load( "conf/server.conf" ) != 0 ){
		fprintf( stderr, "Failed to load configuration. Exiting.\n" );
		return EXIT_FAILURE;
	}

	logger_init( global_config.log_file );
	logger_log( "Starting kiwi-http web server..." );

	cache_init( &global_cache, 100 );
	threadpool_init( &global_pool, global_config.thread_count );

	server_start();

	threadpool_destroy( &global_pool );
	cache_destroy( &global_cache );
	logger_destroy();
	
	return EXIT_SUCCESS;
}
