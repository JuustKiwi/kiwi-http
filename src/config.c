#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/config.h"

server_config_t global_config;

int config_load( const char *filepath ){
	FILE *file;
	char line[ 512 ];

	global_config.port = 8080;
	global_config.thread_count = 4;
	strncpy( global_config.document_root, "./www", sizeof( global_config.document_root ) );
	strncpy( global_config.log_file, "./logs/access.log", sizeof( global_config.log_file ) );

	file = fopen( filepath, "r" );
	if( file == NULL ){
		fprintf( stderr, "kiwi-http: Warning: Could not open config '%s'. Using defaults.\n", filepath );
		return -1;
	}

	while( fgets( line, sizeof( line ), file ) ){
		char *key;
		char *val;
		char *newline;

		if( line[ 0 ] == '#' || line[ 0 ] == '\n' ){
			continue;
		}

		key = strtok( line, "=" );
		val = strtok( NULL, "=" );

		if( key != NULL && val != NULL ){
			newline = strchr( val, '\n' );
			if( newline ){
				*newline = '\0';
			}

			if( strcmp( key, "port" ) == 0 ){
				global_config.port = ( uint16_t ) atoi( val );
			} else if( strcmp( key, "threads" ) == 0 ){
				global_config.thread_count = atoi( val );
			} else if( strcmp( key, "document_root" ) == 0 ){
				strncpy( global_config.document_root, val, sizeof( global_config.document_root ) - 1 );
			} else if( strcmp( key, "log_file" ) == 0 ){
				strncpy( global_config.log_file, val, sizeof( global_config.log_file ) - 1 );
			}
		}
	}

	fclose( file );
	return 0;
}
