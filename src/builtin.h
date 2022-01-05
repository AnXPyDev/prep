#include <malloc.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef void *(*builtin_fn)(io_interface_t*, token_store_t*, vector_t*);

void *builtin_pushs(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 2 ) {
		fprintf(stderr, "missing arguments for builtin pushs\n");
		return NULL;
	}

	wstring_t *name = (wstring_t*)vector_get(args, 0);
	wstring_t *val = (wstring_t*)vector_get(args, 1);

	token_t *token = ensure_token(store, name);
	wstring_t *content = malloc(sizeof(wstring_t));
	wstring_init_copy(content, val);
	token_def_t *def = token_def_create(token_string, (void*)content);
	token_pushdef(token, def);

	return NULL;
}

void *builtin_pop(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 1 ) {
		fprintf(stderr, "missing arguments for builtin pop\n");
		return NULL;
	}

	wstring_t *name = (wstring_t*)vector_get(args, 0);

	token_t *token = ensure_token(store, name);
	token_popdef(token);

	return NULL;
}

void *builtin_pushm(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 3 ) {
		fprintf(stderr, "missing arguments for builtin pushm\n");
		return NULL;
	}

	wstring_t *name = (wstring_t*)vector_get(args, 0);

	wstring_t *val = (wstring_t*)vector_get(args, args->size - 1);

	token_t *token = ensure_token(store, name);

	macro_payload_t *macro_pl = macro_create_payload();

	wstring_init_copy(&macro_pl->content, val);

	for (unsigned int i = 1; i < args->size - 1; i++ ) {
		wstring_t *arg = (wstring_t*)vector_push_blank(&macro_pl->arguments);
		wstring_init_copy(arg, (wstring_t*)vector_get(args, i));
	}

	token_def_t *def = token_def_create(token_macro, (void*)macro_pl);
	token_pushdef(token, def);

	return NULL;
}

void *builtin_include(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 1 ) {
		fprintf(stderr, "missing arguments for builtin include\n");
		return NULL;
	}

	wstring_t *path = (wstring_t*)vector_get(args, 0);
	
	char cpath[1024];
	char cdir[1024];

	wcstombs(cpath, path->data, 1024);

	FILE *fp = NULL;

	for ( unsigned int i = include_directories.size; i > 0; i-- ) {
		wstring_t *dir = (wstring_t*)vector_get(&include_directories, i - 1);
		wcstombs(cdir, dir->data, 1024);

		strcat(cdir, cpath);
		fp = fopen(cdir, "r");
		if ( fp != NULL ) {
			if ( debug ) {
				fprintf(stderr, "found file at %s\n", cdir);
			}
			break;
		}
	}

	if ( fp == NULL ) {
		fprintf(stderr, "cannot open file \"%s\" for inclusion\n", cpath);
		return NULL;
	}

	io_interface_t sub_io = *io;

	sub_io.in_payload = (void*)fp;
	sub_io.get = io_get_file;
	sub_io.unget = io_unget_file;
	sub_io.eof = io_eof_file;
	
	sub_base(&sub_io);

	fclose(fp);

	return NULL;
}

void *builtin_ifdef(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 2 ) {
		fprintf(stderr, "missing arguments for builtin ifdef\n");
		return NULL;
	}

	wstring_t *name = (wstring_t*)vector_get(args, 0);

	token_t *token = get_token(store, name);


	if ( token && token->def ) {
		io_interface_t sub_io = *io;
		sub_io.get = io_get_string;
		sub_io.unget = io_unget_string;
		sub_io.eof = io_eof_string;
		
		wstring_t *result = (wstring_t*)vector_get(args, 1);

		wchar_t *string_reader = result->data;

		sub_io.in_payload = (void*)&string_reader;

		sub_base(&sub_io);
	} else if ( args->size > 2 ) { 
		io_interface_t sub_io = *io;
		sub_io.get = io_get_string;
		sub_io.unget = io_unget_string;
		sub_io.eof = io_eof_string;
		
		wstring_t *result = (wstring_t*)vector_get(args, 2);

		wchar_t *string_reader = result->data;

		sub_io.in_payload = (void*)&string_reader;

		sub_base(&sub_io);

	}

	return NULL;
}

void *builtin_ifndef(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 2 ) {
		fprintf(stderr, "missing arguments for builtin ifdef\n");
		return NULL;
	}

	wstring_t *name = (wstring_t*)vector_get(args, 0);

	token_t *token = get_token(store, name);


	if ( !token || !token->def ) {
		io_interface_t sub_io = *io;
		sub_io.get = io_get_string;
		sub_io.unget = io_unget_string;
		sub_io.eof = io_eof_string;
		
		wstring_t *result = (wstring_t*)vector_get(args, 1);

		wchar_t *string_reader = result->data;

		sub_io.in_payload = (void*)&string_reader;

		sub_base(&sub_io);
	} else if ( args->size > 2 ) { 
		io_interface_t sub_io = *io;
		sub_io.get = io_get_string;
		sub_io.unget = io_unget_string;
		sub_io.eof = io_eof_string;
		
		wstring_t *result = (wstring_t*)vector_get(args, 2);

		wchar_t *string_reader = result->data;

		sub_io.in_payload = (void*)&string_reader;

		sub_base(&sub_io);

	}

	return NULL;
}

void setup_builtins(token_store_t *store) {
	{
		wstring_t key;
		wstring_init(&key, L"pushs");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_pushs);
		token_pushdef(macro, def);
		wstring_destroy(&key);
	}

	{
		wstring_t key;
		wstring_init(&key, L"pop");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_pop);
		token_pushdef(macro, def);
		wstring_destroy(&key);
	}
	
	{
		wstring_t key;
		wstring_init(&key, L"pushm");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_pushm);
		token_pushdef(macro, def);
		wstring_destroy(&key);
	}
	
	{
		wstring_t key;
		wstring_init(&key, L"include");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_include);
		token_pushdef(macro, def);
		wstring_destroy(&key);
	}

	{
		wstring_t key;
		wstring_init(&key, L"ifdef");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_ifdef);
		token_pushdef(macro, def);
		wstring_destroy(&key);
	}

	{
		wstring_t key;
		wstring_init(&key, L"ifndef");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_ifndef);
		token_pushdef(macro, def);
		wstring_destroy(&key);
	}
}
