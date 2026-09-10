#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../include/server.h"
#include "../include/config.h"
#include "../include/threadpool.h"
#include "../include/logger.h"
#include "../include/http.h"

extern threadpool_t global_pool;
static int server_socket = -1;
static volatile int server_running = 1;

void server_stop( void ){
	server_running = 0;
	if( server_socket != -1 ){
		close( server_socket );
	}
}

void server_start( void ){
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	socklen_t client_len;
	int client_socket;
	int opt;

	server_socket = socket( AF_INET, SOCK_STREAM, 0 );
	if( server_socket < 0 ){
		perror( "kiwi-http: socket creation failed" );
		exit( EXIT_FAILURE );
	}

	opt = 1;
	if( setsockopt( server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof( opt ) ) < 0 ){
		perror( "kiwi-http: setsockopt failed" );
		exit( EXIT_FAILURE );
	}

	memset( &server_addr, 0, sizeof( server_addr ) );
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons( global_config.port );

	if( bind( server_socket, ( struct sockaddr * ) &server_addr, sizeof( server_addr ) ) < 0 ){
		perror( "kiwi-http: bind failed" );
		exit( EXIT_FAILURE );
	}

	if( listen( server_socket, 1024 ) < 0 ){
		perror( "kiwi-http: listen failed" );
		exit( EXIT_FAILURE );
	}

	logger_log( "Server listening on port %d...", global_config.port );

	while( server_running ){
		client_len = sizeof( client_addr );
		client_socket = accept( server_socket, ( struct sockaddr * ) &client_addr, &client_len );
		
		if( client_socket < 0 ){
			if( server_running ){
				perror( "kiwi-http: accept error" );
			}
			continue;
		}

		threadpool_submit( &global_pool, http_handle_client, client_socket );
	}

	logger_log( "Server loop terminated." );
}
