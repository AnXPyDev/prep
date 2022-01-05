#include <wchar.h>

typedef enum {
	token_null,
	token_string,
	token_builtin,
	token_list,
	token_bool,
	token_num,
	token_macro
} token_type_e ;

typedef struct token_def_s {
	struct token_def_s *prev_def;
	token_type_e type;
	void *payload;
} token_def_t;

token_def_t *token_def_create(token_type_e type, void *payload) {
	token_def_t *result = malloc(sizeof(token_def_t));
	result->prev_def = NULL;
	result->type = type;
	result->payload = payload;

	return result;
}

int token_def_destroy(token_def_t *def) {
	if ( def->type == token_string ) {
		wstring_destroy((wstring_t*)def->payload);
		free(def->payload);
	} else if ( def->type == token_macro ) {
		macro_destroy_payload((macro_payload_t*)def->payload);
	}

	free(def);

	return 0;
}

int token_def_destroy_stack(token_def_t *def) {
	if ( def->prev_def != NULL ) {
		token_def_destroy_stack(def->prev_def);
	}

	token_def_destroy(def);
}

typedef struct {
	wstring_t name;
	token_def_t *def;
} token_t;

int token_destroy(token_t *token) {
	wstring_destroy(&token->name);
	
	if ( token->def != NULL ) {
		token_def_destroy_stack(token->def);
	}

	token->def = NULL;

	free(token);
	
	return 0;
}

typedef struct {
	vector_t tokens;
	vector_t match_buffer;
	unsigned int wc_count;
	hashmap_t map;
} token_store_t;

int token_store_init(token_store_t *store) {
	vector_init(&store->tokens, 5, sizeof(token_t*));
	vector_init(&store->match_buffer, 5, sizeof(int));
	hashmap_init(&store->map, sizeof(token_t*));
	store->wc_count = 0;
	return 0;
}

int token_store_destroy(token_store_t *store) {
	for ( unsigned int i = 0; i < store->tokens.size; i++ ) {
		token_t *token = *(token_t**)vector_get(&store->tokens, i);
		token_destroy(token);
	}
	vector_destroy(&store->tokens);
	vector_destroy(&store->match_buffer);
	hashmap_destroy(&store->map);
	return 0;
}


int token_pushdef(token_t *token, token_def_t *def) {
	def->prev_def = token->def;
	token->def = def;
}

int token_popdef(token_t *token) {
	if ( token->def == NULL ) {
		return 1;
	}

	token_def_t *prev_def = token->def->prev_def;
	token_def_destroy(token->def);
	token->def = prev_def;
	return 0;	
}

token_t *register_token_blank(token_store_t *store, const wstring_t *name) {
	token_t *token = malloc(sizeof(token_t));
	*(token_t**)vector_push_blank(&store->tokens) = token;
	*(int*)vector_push_blank(&store->match_buffer) = 0;
	*(token_t**)hashmap_push_blank_ws(&store->map, name) = token;
	wstring_init_copy(&token->name, name);
	token->def = NULL;
	return token;
}

token_t *register_token_string(token_store_t *store, const wstring_t *name, wstring_t *content) {
	token_t *token = register_token_blank(store, name);
	wstring_t *content_copy = malloc(sizeof(wstring_t));
	wstring_init_copy(content_copy, content);
	token_pushdef(token, token_def_create(token_string, content_copy));
	return token;
}

int match_token_wc(token_store_t *store, wchar_t wc) {
	for ( unsigned int i = 0; i < store->tokens.size; i++ ) {
		int *match = (int*)vector_get(&store->match_buffer, i);

		if ( *match == -1 ) {
			continue;
		}

		token_t *token = *(token_t**)vector_get(&store->tokens, i);

		if ( token->name.data[store->wc_count] != wc || token->def == NULL ) {
			*match = -1;
		}

	}

	store->wc_count++;

	return 0;
}

token_t *get_token(const token_store_t *store, const wstring_t *name) {
	return hashmap_get_ws(&store->map, name);
}

token_t *ensure_token(token_store_t *store, const wstring_t *name) {
	token_t *token = get_token(store, name);

	if ( token == NULL ) {
		return register_token_blank(store, name);
	}

	return token;
}

token_t *get_token_match(token_store_t *store) {
	for ( unsigned int i = 0; i < store->tokens.size; i++ ) {
		int *match = (int*)vector_get(&store->match_buffer, i);
	
		if ( *match == 0 ) {
			return *(token_t**)vector_get(&store->tokens, i);
		}
	}

	return NULL;
}


int reset_token_match(token_store_t *store) {
	if ( store->wc_count != 0 ) {
		store->wc_count = 0;
		memset(store->match_buffer.data, 0, store->match_buffer.data_size * store->match_buffer.size);
	}
	return 0;
}
