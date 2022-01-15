vector_t *stringlist_create_payload() {
	vector_t *result = malloc(sizeof(vector_t));
	vector_init(result, 2, sizeof(wstring_t));
	return result;
}

void stringlist_destroy_payload(vector_t *vec) {
	for ( unsigned int i = 0; i < vec->size; i++ ) {
		wstring_destroy((wstring_t*)vector_get(vec, i));
	}

	vector_destroy(vec);
	free(vec);
}
