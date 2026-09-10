#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../include/cache.h"
#include "../include/logger.h"

lru_cache_t global_cache;

static uint64_t hash_string( const char *str ){
	uint64_t hash;
	int i;

	hash = 14695981039346656037ULL;
	i = 0;

	while( str[ i ] != '\0' ){
		hash ^= ( unsigned char ) str[ i ];
		hash *= 1099511628211ULL;
		i++;
	}
	return hash;
}

static void move_to_head( lru_cache_t *cache, cache_node_t *node ){
	if( cache->head == node ){
		return;
	}

	if( node->prev != NULL ){
		node->prev->next = node->next;
	}
	if( node->next != NULL ){
		node->next->prev = node->prev;
	}
	if( cache->tail == node ){
		cache->tail = node->prev;
	}

	node->next = cache->head;
	node->prev = NULL;

	if( cache->head != NULL ){
		cache->head->prev = node;
	}
	cache->head = node;

	if( cache->tail == NULL ){
		cache->tail = node;
	}
}

void cache_init( lru_cache_t *cache, int capacity ){
	cache->capacity = capacity;
	cache->count = 0;
	cache->head = NULL;
	cache->tail = NULL;
	cache->num_buckets = capacity * 2;
	
	cache->buckets = calloc( cache->num_buckets, sizeof( cache_node_t * ) );
	pthread_mutex_init( &cache->lock, NULL );
}

cache_node_t *cache_get( lru_cache_t *cache, const char *filepath ){
	uint64_t hash;
	int index;
	cache_node_t *curr;

	pthread_mutex_lock( &cache->lock );
	
	hash = hash_string( filepath );
	index = hash % cache->num_buckets;
	curr = cache->buckets[ index ];

	while( curr != NULL ){
		if( strcmp( curr->filepath, filepath ) == 0 ){
			move_to_head( cache, curr );
			curr->ref_count++;
			pthread_mutex_unlock( &cache->lock );
			return curr;
		}
		curr = curr->hnext;
	}

	pthread_mutex_unlock( &cache->lock );
	return NULL;
}

void cache_release( lru_cache_t *cache, cache_node_t *node ){
	if( node == NULL ){
		return;
	}

	pthread_mutex_lock( &cache->lock );
	
	node->ref_count--;
	
	if( node->ref_count == 0 && node->is_deleted ){
		free( node->filepath );
		free( node->data );
		free( node );
	}
	
	pthread_mutex_unlock( &cache->lock );
}

void cache_put( lru_cache_t *cache, const char *filepath, const char *data, size_t size ){
	uint64_t hash;
	int index;
	cache_node_t *new_node;
	cache_node_t *evict;

	pthread_mutex_lock( &cache->lock );

	if( cache->count >= cache->capacity ){
		evict = cache->tail;
		if( evict != NULL ){
			if( evict->prev != NULL ){
				evict->prev->next = NULL;
			}
			cache->tail = evict->prev;
			if( cache->head == evict ){
				cache->head = NULL;
			}

			hash = hash_string( evict->filepath );
			index = hash % cache->num_buckets;
			
			cache_node_t *curr = cache->buckets[ index ];
			cache_node_t *prev = NULL;
			
			while( curr != NULL ){
				if( curr == evict ){
					if( prev == NULL ){
						cache->buckets[ index ] = curr->hnext;
					} else {
						prev->hnext = curr->hnext;
					}
					break;
				}
				prev = curr;
				curr = curr->hnext;
			}

			evict->is_deleted = 1;
			if( evict->ref_count == 0 ){
				free( evict->filepath );
				free( evict->data );
				free( evict );
			}
			cache->count--;
		}
	}

	new_node = malloc( sizeof( cache_node_t ) );
	new_node->filepath = strdup( filepath );
	new_node->data = malloc( size );
	memcpy( new_node->data, data, size );
	new_node->size = size;
	new_node->ref_count = 0;
	new_node->is_deleted = 0;

	new_node->next = cache->head;
	new_node->prev = NULL;
	if( cache->head != NULL ){
		cache->head->prev = new_node;
	}
	cache->head = new_node;
	if( cache->tail == NULL ){
		cache->tail = new_node;
	}

	hash = hash_string( filepath );
	index = hash % cache->num_buckets;
	new_node->hnext = cache->buckets[ index ];
	cache->buckets[ index ] = new_node;
	
	cache->count++;

	pthread_mutex_unlock( &cache->lock );
}

void cache_destroy( lru_cache_t *cache ){
	cache_node_t *curr;
	cache_node_t *next;

	pthread_mutex_lock( &cache->lock );
	curr = cache->head;
	while( curr != NULL ){
		next = curr->next;
		free( curr->filepath );
		free( curr->data );
		free( curr );
		curr = next;
	}
	free( cache->buckets );
	pthread_mutex_unlock( &cache->lock );
	pthread_mutex_destroy( &cache->lock );
}
