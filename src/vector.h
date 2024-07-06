#include <malloc.h>
#include <string.h>

typedef struct {
	void *data;
	unsigned int size;
	unsigned int capacity;
	unsigned int data_size;
} vector_t;

vector_t vector_create(unsigned int capacity, unsigned int data_size) {
	vector_t result;

	result.size = 0;	
	result.data_size = data_size;
	result.data = malloc(capacity * data_size);
	result.capacity = capacity;

	return result;
}

int vector_init(vector_t *vector, unsigned int capacity, unsigned int data_size) {
	vector->size = 0;	
	vector->data_size = data_size;
	vector->data = malloc(capacity * data_size);
	vector->capacity = capacity;
	return 0;
}

int vector_destroy(vector_t *vector) {
	free(vector->data);
	vector->capacity = 0;
	vector->size = 0;
	return 0;
}

void *vector_get(const vector_t *vector, unsigned int index) {
	return vector->data + index * vector->data_size;
}

int vector_resize(vector_t *vector, unsigned int new_capacity) {
	void *new_data = malloc(vector->data_size * new_capacity);
	if ( vector->size > new_capacity ) {
		memcpy(new_data, vector->data, vector->data_size * new_capacity);
	} else {
		memcpy(new_data, vector->data, vector->data_size * vector->size);
	}
	
	free(vector->data);
	vector->data = new_data;

	vector->capacity = new_capacity;

	return 0;
}

void *vector_push_blank(vector_t *vector) {
	if ( vector->size + 1 > vector->capacity ) {
		vector_resize(vector, vector->capacity * 1.5 + 1);
	}
	
	vector->size++;
	return vector->data + (vector->size - 1) * vector->data_size;
}

int vector_push(vector_t *vector, const void *data) {
	void *blank = vector_push_blank(vector);

	memcpy(blank, data, vector->data_size);

	return 0; 
}

