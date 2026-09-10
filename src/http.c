#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "../include/http.h"
#include "../include/config.h"
#include "../include/logger.h"
#include "../include/mime.h"
#include "../include/cache.h"

#define MAX_CACHE_FILE 1048576 

void http_send_404( int client_socket, int keep_alive ){
	char response[ 512 ];

	snprintf( response, sizeof( response ),
	          "HTTP/1.1 404 Not Found\r\n"
	          "Content-Type: text/html\r\n"
	          "Content-Length: 48\r\n"
	          "Connection: %s\r\n\r\n"
	          "<html><body><h1>404 Not Found</h1></body></html>",
	          keep_alive ? "keep-alive" : "close" );
	          
	send( client_socket, response, strlen( response ), 0 );
}

void http_handle_client( int client_socket ){
	struct timeval tv;

	tv.tv_sec = 5;
	tv.tv_usec = 0;
	if( setsockopt( client_socket, SOL_SOCKET, SO_RCVTIMEO, ( const char * ) &tv, sizeof( tv ) ) < 0 ){
		perror( "kiwi-http: setsockopt timeout failed" );
	}

	while( 1 ){
		char buffer[ 4096 ];
		ssize_t bytes_read;
		char *method;
		char *uri;
		char *version;
		char filepath[ 2048 ];
		struct stat file_stat;
		int fd;
		char header[ 1024 ];
		const char *mime_type;
		cache_node_t *cached_file;
		int keep_alive;

		memset( buffer, 0, sizeof( buffer ) );
		bytes_read = recv( client_socket, buffer, sizeof( buffer ) - 1, 0 );

		if( bytes_read <= 0 ){
			break;
		}

		keep_alive = 1;
		if( strstr( buffer, "Connection: close" ) != NULL || 
		    strstr( buffer, "Connection: Close" ) != NULL ){
			keep_alive = 0;
		}

		method = strtok( buffer, " " );
		uri = strtok( NULL, " " );
		version = strtok( NULL, "\r\n" );

		if( method == NULL || uri == NULL || version == NULL ){
			break;
		}

		if( strstr( uri, "../" ) != NULL ){
			logger_log( "SECURITY: Directory traversal attempt blocked for URI: %s", uri );
			http_send_404( client_socket, keep_alive );
			if( !keep_alive ) break;
			continue;
		}

		if( strcmp( uri, "/" ) == 0 ){
			snprintf( filepath, sizeof( filepath ), "%s/index.html", global_config.document_root );
		} else {
			snprintf( filepath, sizeof( filepath ), "%s%s", global_config.document_root, uri );
		}

		mime_type = mime_get_type( filepath );
		cached_file = cache_get( &global_cache, filepath );

		if( cached_file != NULL ){
			snprintf( header, sizeof( header ), 
			          "HTTP/1.1 200 OK\r\n"
			          "Content-Type: %s\r\n"
			          "Content-Length: %ld\r\n"
			          "Connection: %s\r\n\r\n", 
			          mime_type, cached_file->size, keep_alive ? "keep-alive" : "close" );

			send( client_socket, header, strlen( header ), 0 );
			send( client_socket, cached_file->data, cached_file->size, 0 );
			
//			logger_log( "200 OK [CACHE HIT] - %s %s", method, uri );
			cache_release( &global_cache, cached_file );
			
			if( !keep_alive ) break;
			continue;
		}

		if( stat( filepath, &file_stat ) < 0 || S_ISDIR( file_stat.st_mode ) ){
			logger_log( "404 Not Found - %s %s", method, uri );
			http_send_404( client_socket, keep_alive );
			if( !keep_alive ) break;
			continue;
		}

		fd = open( filepath, O_RDONLY );
		if( fd < 0 ){
			http_send_404( client_socket, keep_alive );
			if( !keep_alive ) break;
			continue;
		}

		snprintf( header, sizeof( header ), 
		          "HTTP/1.1 200 OK\r\n"
		          "Content-Type: %s\r\n"
		          "Content-Length: %ld\r\n"
		          "Connection: %s\r\n\r\n", 
		          mime_type, file_stat.st_size, keep_alive ? "keep-alive" : "close" );

		send( client_socket, header, strlen( header ), 0 );

		if( file_stat.st_size < MAX_CACHE_FILE ){
			char *file_buf;
			ssize_t read_bytes;
			
			file_buf = malloc( file_stat.st_size );
			read_bytes = read( fd, file_buf, file_stat.st_size );
			
			if( read_bytes == file_stat.st_size ){
				send( client_socket, file_buf, file_stat.st_size, 0 );
				cache_put( &global_cache, filepath, file_buf, file_stat.st_size );
//				logger_log( "200 OK [CACHE MISS -> STORED] - %s %s", method, uri );
			}
			free( file_buf );
		} else {
			char chunk[ 8192 ];
			ssize_t bytes;
			
			while( ( bytes = read( fd, chunk, sizeof( chunk ) ) ) > 0 ){
				send( client_socket, chunk, bytes, 0 );
			}
//			logger_log( "200 OK [FILE TOO LARGE FOR CACHE] - %s %s", method, uri );
		}

		close( fd );
		
		if( !keep_alive ){
			break;
		}
	}

	close( client_socket );
}
