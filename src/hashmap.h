#include <string.h>
#include <malloc.h>

#define HASHMAP_SIZE 32

typedef struct hashmap_node_s {
	struct hashmap_node_s *next_node;
	unsigned int index_size;
	char *index;
} hashmap_node_t;

typedef struct {
	unsigned int data_size;
	hashmap_node_t *heads[HASHMAP_SIZE];
} hashmap_t;

unsigned int hashmap_hash(const char *data, unsigned int size) {
	unsigned int hash = 5381;
	
	while ( size != 0 ) {
        	hash = ((hash << 5) + hash) + *data;
		data++;
		size--;
	}

    	return hash;
}

hashmap_t hashmap_create(unsigned int data_size) {
	hashmap_t result;

	result.data_size = data_size;

	for ( unsigned int i = 0; i < HASHMAP_SIZE; i++ ) {
		result.heads[i] = NULL;
	}

	return result;
}

int hashmap_init(hashmap_t *hashmap, unsigned int data_size) {
	hashmap->data_size = data_size;

	for ( unsigned int i = 0; i < HASHMAP_SIZE; i++ ) {
		hashmap->heads[i] = NULL;
	}

	return 0;
}

hashmap_node_t *hashmap_create_node(const hashmap_t *hashmap, const char *index, unsigned int index_size) {
	hashmap_node_t *result = malloc(sizeof(hashmap_node_t) + hashmap->data_size);
	result->index = malloc(index_size);
	memcpy(result->index, index, index_size);
	result->index_size = index_size;
	result->next_node = NULL;
	memset((void*)result + sizeof(hashmap_node_t), 1, hashmap->data_size);
	return result;
}

int hashmap_destroy_node(hashmap_node_t *node) {
	free(node->index);
	free(node);
	return 0;
}

int hashmap_destroy_node_list(hashmap_node_t *node) {
	if ( node == NULL ) {
		return 0;
	}

	hashmap_destroy_node_list(node->next_node);
	hashmap_destroy_node(node);
	return 0;
}


int hashmap_destroy(hashmap_t *hashmap) {
	
	for ( unsigned int i = 0; i < HASHMAP_SIZE; i++ ) {
		hashmap_destroy_node_list(hashmap->heads[i]);
		hashmap->heads[i] = NULL;
	}

	hashmap->data_size = 0;

	return 0;
}

void *hashmap_push_blank(hashmap_t *hashmap, const char *index, unsigned int index_size) {
	unsigned int ix = hashmap_hash(index, index_size) % (HASHMAP_SIZE + 1);

	hashmap_node_t *new_node = hashmap_create_node(hashmap, index, index_size);

	hashmap_node_t **node = hashmap->heads + ix;
	while ( 1 ) {
		if ( *node == NULL ) {
			*node = new_node;
			break;
		}
		node = (hashmap_node_t**)*node;
	}

	return (void*)new_node + sizeof(hashmap_node_t);
}

hashmap_node_t **hashmap_get_node(hashmap_t *hashmap, const char *index, unsigned int index_size) {
	unsigned int ix = hashmap_hash(index, index_size) % (HASHMAP_SIZE + 1);

	// qualifier not dicarded, hashmap does not get mutated
	hashmap_node_t **node = hashmap->heads + ix;
	while ( 1 ) {
		if ( *node == NULL ) {
			return NULL;
		} else if ( (*node)->index_size == index_size && memcmp((*node)->index, index, index_size) == 0 ) {
			return node;
		} else {
			node = (hashmap_node_t**)*node;
		}
	}
}

void *hashmap_get(hashmap_t *hashmap, const char *index, unsigned int index_size) {
	hashmap_node_t **node = hashmap_get_node(hashmap, index, index_size);

	if ( node == NULL ) {
		return NULL;
	} else {
		return (void*)(*node) + sizeof(hashmap_node_t);
	}
}

int hashmap_remove(hashmap_t *hashmap, const char *index, unsigned int index_size) {
	hashmap_node_t **node = hashmap_get_node(hashmap, index, index_size);
	
	if ( node == NULL ) {
		return 1;
	}

	hashmap_node_t *next_node = (*node)->next_node;

	hashmap_destroy_node(*node);

	*node = next_node;

	return 0;
}

void *hashmap_push_blank_ws(hashmap_t *hashmap, const wstring_t *key) {
	return hashmap_push_blank(hashmap, (char*)key->data, key->size * sizeof(wchar_t));
}

void *hashmap_get_ws(hashmap_t *hashmap, const wstring_t *key) {
	return hashmap_get(hashmap, (char*)key->data, key->size * sizeof(wchar_t));
}

int hashmap_remove_ws(hashmap_t *hashmap, const wstring_t *key) {
	return hashmap_remove(hashmap, (char*)key->data, key->size * sizeof(wchar_t));
}


