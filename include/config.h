#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

typedef struct {
	uint16_t port;
	int thread_count;
	char document_root[ 1024 ];
	char log_file[ 1024 ];
} server_config_t;

extern server_config_t global_config;

int config_load( const char *filepath );

#endif
