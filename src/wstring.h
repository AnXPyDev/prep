#include <string.h>
#include <wchar.h>

const wchar_t NULL_WCHAR = L'\0';

typedef struct {
	wchar_t *data;
	unsigned int size;
	unsigned int capacity;
} wstring_t;

wstring_t wstring_create_blank(unsigned int capacity) {
	wstring_t result;
	result.data = malloc(sizeof(wchar_t) * capacity);
	result.data[0] = NULL_WCHAR;
	result.size = 0;
	result.capacity = 0;

	return result;
}

int wstring_init_blank(wstring_t *wstring, unsigned int capacity) {
	wstring->data = malloc(sizeof(wchar_t) * capacity);
	wstring->data[0] = NULL_WCHAR;
	wstring->size = 0;
	wstring->capacity = 0;
	
	return 0;
}

wstring_t wstring_create(const wchar_t *source_string) {
	wstring_t result;
	result.size = wcslen(source_string);
	result.capacity = result.size * 1.5;
	result.data = malloc(sizeof(wchar_t) * (result.capacity + 1));
	memcpy(result.data, source_string, sizeof(wchar_t) * (result.size + 1));

	return result;
}

int wstring_init(wstring_t *wstring, const wchar_t *source_string) {
	wstring->size = wcslen(source_string);
	wstring->capacity = wstring->size * 1.5;
	wstring->data = malloc(sizeof(wchar_t) * (wstring->capacity + 1));
	memcpy(wstring->data, source_string, sizeof(wchar_t) * (wstring->size + 1));
	
	return 0;
}

wstring_t wstring_create_copy(const wstring_t *source) {
	wstring_t result;
	result.size = source->size;
	result.capacity = source->capacity;
	result.data = malloc(sizeof(wchar_t) * (result.capacity + 1));
	memcpy(result.data, source->data, sizeof(wchar_t) * (result.size + 1));
	
	return result;
}

int wstring_init_copy(wstring_t *wstring, const wstring_t *source) {
	wstring->size = source->size;
	wstring->capacity = source->capacity;
	wstring->data = malloc(sizeof(wchar_t) * (wstring->capacity + 1));
	memcpy(wstring->data, source->data, sizeof(wchar_t) * (wstring->size + 1));

	return 0;
}

int wstring_destroy(wstring_t *wstring) {
	if ( wstring->data != NULL ) {
		free(wstring->data);
		wstring->data = NULL;
	}
	wstring->size = 0;
	wstring->capacity = 0;

	
	return 0;
}

int wstring_resize(wstring_t *wstring, unsigned int new_capacity) {
	wchar_t *new_data = malloc(sizeof(wchar_t) * (new_capacity + 1));

	if ( wstring->size + 1 > new_capacity ) {
		memcpy(new_data, wstring->data, sizeof(wchar_t) * (new_capacity - 1));
		new_data[new_capacity - 1] = NULL_WCHAR;
		wstring->size = new_capacity;
	} else {
		memcpy(new_data, wstring->data, sizeof(wchar_t) * (wstring->size + 1));
	}

	free(wstring->data);
	wstring->data = new_data;
	wstring->capacity = new_capacity;

	return 0;
}

int wstring_putwc(wstring_t *wstring, wchar_t wc) {
	if ( wstring->size + 1 > wstring->capacity ) {
		wstring_resize(wstring, (wstring->capacity + 1) * 1.5);
	}

	wstring->data[wstring->size] = wc;
	wstring->size++;
	wstring->data[wstring->size] = NULL_WCHAR;

	return 0;
}

int wstring_cat(wstring_t *wstring, const wstring_t *other) {
	if ( wstring->size + other->size > wstring->capacity ) {
		wstring_resize(wstring, wstring->capacity + other->capacity);
	}
	memcpy(wstring->data + wstring->size, other->data, other->size + 1);
	wstring->size += other->size;

	return 0;
}

int wstring_cat_raw(wstring_t *wstring, const wchar_t *other) {
	unsigned int other_size = wcslen(other);
	if ( wstring->size + other_size > wstring->capacity ) {
		wstring_resize(wstring, wstring->capacity + other_size);
	}
	memcpy(wstring->data + wstring->size, other, other_size + 1);
	wstring->size += other_size;

	return 0;
}

int wstring_set_size(wstring_t *wstring, unsigned int size) {
	wstring->size = size;
	wstring->data[wstring->size] = NULL_WCHAR;
	return 0;
}

int wstring_reload_size(wstring_t *wstring) {
	wstring->size = wcslen(wstring->data);
	return 0;
}

int wstring_contains_wc(const wstring_t *wstring, wchar_t wc) {
	for ( unsigned int i = 0; i < wstring->size; i++ ) {
		if ( wstring->data[i] == wc ) {
			return 1;
		}
	}

	return 0;
}

int wstring_equal(const wstring_t *wstring, const wstring_t *other) {
	if ( wstring->size != other->size ) {
		return 0;
	}

	for ( unsigned int i = 0; i < wstring->size; i++ ) {
		if ( wstring->data[i] != other->data[i] ) {
			return 0;
		}
	}
	
	return 1;
}

int wstring_write(const wstring_t *wstring, io_interface_t *io) {
	for ( unsigned int i = 0; i < wstring->size; i++ ) {
		io_put(io, wstring->data[i]);
	}
	return 0;
}

int wstring_strip_trailing(wstring_t *wstring, wchar_t wc) {
	unsigned int new_size = wstring->size;

	while ( 1 ) {
		if ( wstring->data[new_size - 1] == wc ) {
			new_size--;
		} else {
			break;
		}
	}

	wstring_set_size(wstring, new_size);
	return 0;
}


