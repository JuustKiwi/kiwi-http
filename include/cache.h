#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>
#include <pthread.h>

typedef struct cache_node {
	char *filepath;
	char *data;
	size_t size;
	int ref_count;
	int is_deleted;
	struct cache_node *prev;
	struct cache_node *next;
	struct cache_node *hnext;
} cache_node_t;

typedef struct {
	int capacity;
	int count;
	cache_node_t *head;
	cache_node_t *tail;
	cache_node_t **buckets;
	int num_buckets;
	pthread_mutex_t lock;
} lru_cache_t;

extern lru_cache_t global_cache;

void cache_init( lru_cache_t *cache, int capacity );
cache_node_t *cache_get( lru_cache_t *cache, const char *filepath );
void cache_release( lru_cache_t *cache, cache_node_t *node );
void cache_put( lru_cache_t *cache, const char *filepath, const char *data, size_t size );
void cache_destroy( lru_cache_t *cache );

#endif
