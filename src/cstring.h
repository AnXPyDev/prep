#include <string.h>
#include <wchar.h>
#include <malloc.h>



#define LIBEC_CSTRING_INCLUDED
const char NULL_CHAR = 0;

typedef struct {
	char *data;
	unsigned int size;
	unsigned int capacity;
} cstring_t;

cstring_t cstring_create_blank(unsigned int capacity) {
	cstring_t result;
	result.data = malloc(sizeof(char) * capacity);
	result.data[0] = NULL_CHAR;
	result.size = 0;
	result.capacity = 0;

	return result;
}

int cstring_init_blank(cstring_t *cstring, unsigned int capacity) {
	cstring->data = malloc(sizeof(char) * capacity);
	cstring->data[0] = NULL_CHAR;
	cstring->size = 0;
	cstring->capacity = 0;
	
	return 0;
}

cstring_t cstring_create(const char *source_string) {
	cstring_t result;
	result.size = strlen(source_string);
	result.capacity = result.size * 1.5;
	result.data = malloc(sizeof(char) * (result.capacity + 1));
	memcpy(result.data, source_string, sizeof(char) * (result.size + 1));

	return result;
}

int cstring_init(cstring_t *cstring, const char *source_string) {
	cstring->size = strlen(source_string);
	cstring->capacity = cstring->size * 1.5;
	cstring->data = malloc(sizeof(char) * (cstring->capacity + 1));
	memcpy(cstring->data, source_string, sizeof(char) * (cstring->size + 1));
	
	return 0;
}

cstring_t cstring_create_copy(const cstring_t *source) {
	cstring_t result;
	result.size = source->size;
	result.capacity = source->capacity;
	result.data = malloc(sizeof(char) * (result.capacity + 1));
	memcpy(result.data, source->data, sizeof(char) * (result.size + 1));
	
	return result;
}

int cstring_init_copy(cstring_t *cstring, const cstring_t *source) {
	cstring->size = source->size;
	cstring->capacity = source->capacity;
	cstring->data = malloc(sizeof(char) * (cstring->capacity + 1));
	memcpy(cstring->data, source->data, sizeof(char) * (cstring->size + 1));

	return 0;
}

int cstring_destroy(cstring_t *cstring) {
	if ( cstring->data != NULL ) {
		free(cstring->data);
		cstring->data = NULL;
	}
	cstring->size = 0;
	cstring->capacity = 0;

	
	return 0;
}

int cstring_resize(cstring_t *cstring, unsigned int new_capacity) {
	char *new_data = malloc(sizeof(char) * (new_capacity + 1));

	if ( cstring->size + 1 > new_capacity ) {
		memcpy(new_data, cstring->data, sizeof(char) * (new_capacity - 1));
		new_data[new_capacity - 1] = NULL_CHAR;
		cstring->size = new_capacity;
	} else {
		memcpy(new_data, cstring->data, sizeof(char) * (cstring->size + 1));
	}

	free(cstring->data);
	cstring->data = new_data;
	cstring->capacity = new_capacity;

	return 0;
}

int cstring_put(cstring_t *cstring, char wc) {
	if ( cstring->size + 1 > cstring->capacity ) {
		cstring_resize(cstring, (cstring->capacity + 1) * 1.5);
	}

	cstring->data[cstring->size] = wc;
	cstring->size++;
	cstring->data[cstring->size] = NULL_CHAR;

	return 0;
}

int cstring_cat(cstring_t *cstring, const cstring_t *other) {
	if ( cstring->size + other->size > cstring->capacity ) {
		cstring_resize(cstring, cstring->capacity + other->capacity);
	}
	memcpy(cstring->data + cstring->size, other->data, other->size + 1);
	cstring->size += other->size;

	return 0;
}

int cstring_cat_raw(cstring_t *cstring, const char *other) {
	unsigned int other_size = strlen(other);
	if ( cstring->size + other_size > cstring->capacity ) {
		cstring_resize(cstring, cstring->capacity + other_size);
	}
	memcpy(cstring->data + cstring->size, other, other_size + 1);
	cstring->size += other_size;

	return 0;
}

int cstring_set_size(cstring_t *cstring, unsigned int size) {
	cstring->size = size;
	cstring->data[cstring->size] = NULL_CHAR;
	return 0;
}

int cstring_reload_size(cstring_t *cstring) {
	cstring->size = strlen(cstring->data);
	return 0;
}

int cstring_contains(const cstring_t *cstring, char wc) {
	for ( unsigned int i = 0; i < cstring->size; i++ ) {
		if ( cstring->data[i] == wc ) {
			return 1;
		}
	}

	return 0;
}

int cstring_equal(const cstring_t *cstring, const cstring_t *other) {
	if ( cstring->size != other->size ) {
		return 0;
	}

	for ( unsigned int i = 0; i < cstring->size; i++ ) {
		if ( cstring->data[i] != other->data[i] ) {
			return 0;
		}
	}
	
	return 1;
}

int cstring_strip_trailing(cstring_t *cstring, char wc) {
	unsigned int new_size = cstring->size;

	while ( 1 ) {
		if ( cstring->data[new_size - 1] == wc ) {
			new_size--;
		} else {
			break;
		}
	}

	cstring_set_size(cstring, new_size);
	return 0;
}


