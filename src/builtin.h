#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef void *(*builtin_fn)(io_interface_t*, token_store_t*, vector_t*);

void *builtin_pushs(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 2 ) {
		fprintf(stderr, "missing arguments for builtin pushs\n");
		return NULL;
	}

	cstring_t *name = (cstring_t*)vector_get(args, 0);
	cstring_t *val = (cstring_t*)vector_get(args, 1);

	token_t *token = ensure_token(store, name);
	cstring_t *content = malloc(sizeof(cstring_t));
	cstring_init_copy(content, val);
	token_def_t *def = token_def_create(token_string, (void*)content);
	token_pushdef(token, def);


	return NULL;
}

void *builtin_sets(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 2 ) {
		fprintf(stderr, "missing arguments for builtin sets\n");
		return NULL;
	}

	cstring_t *name = (cstring_t*)vector_get(args, 0);
	cstring_t *val = (cstring_t*)vector_get(args, 1);

	token_t *token = ensure_token(store, name);
	cstring_t *content = malloc(sizeof(cstring_t));
	cstring_init_copy(content, val);
	token_def_t *def = token_def_create(token_string, (void*)content);
	token_setdef(token, def);

	return NULL;
}

void *builtin_set(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 1 ) {
		fprintf(stderr, "missing arguments for builtin setdef\n");
		return NULL;
	}

	cstring_t *name = (cstring_t*)vector_get(args, 0);

	token_t *token = ensure_token(store, name);

	if ( token->def ) {
		return NULL;
	}

	token_def_t *def = token_def_create(token_empty, NULL);
	token_setdef(token, def);

	return NULL;
}

void *builtin_pop(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 1 ) {
		fprintf(stderr, "missing arguments for builtin pop\n");
		return NULL;
	}

	cstring_t *name = (cstring_t*)vector_get(args, 0);

	token_t *token = ensure_token(store, name);
	token_popdef(token);

	return NULL;
}

void *builtin_popall(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 1 ) {
		fprintf(stderr, "missing arguments for builtin popall\n");
		return NULL;
	}

	cstring_t *name = (cstring_t*)vector_get(args, 0);

	token_t *token = ensure_token(store, name);
	token_undef(token);

	return NULL;
}

void *builtin_pushm(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 3 ) {
		fprintf(stderr, "missing arguments for builtin pushm\n");
		return NULL;
	}

	cstring_t *name = (cstring_t*)vector_get(args, 0);

	cstring_t *val = (cstring_t*)vector_get(args, args->size - 1);

	token_t *token = ensure_token(store, name);

	macro_payload_t *macro_pl = macro_create_payload();

	cstring_init_copy(&macro_pl->content, val);

	for (unsigned int i = 1; i < args->size - 1; i++ ) {
		cstring_t *arg = (cstring_t*)vector_push_blank(&macro_pl->arguments);
		cstring_init_copy(arg, (cstring_t*)vector_get(args, i));
	}

	token_def_t *def = token_def_create(token_macro, (void*)macro_pl);
	token_pushdef(token, def);

	return NULL;
}

void *builtin_setm(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 3 ) {
		fprintf(stderr, "missing arguments for builtin setm\n");
		return NULL;
	}

	cstring_t *name = (cstring_t*)vector_get(args, 0);

	cstring_t *val = (cstring_t*)vector_get(args, args->size - 1);

	token_t *token = ensure_token(store, name);

	macro_payload_t *macro_pl = macro_create_payload();

	cstring_init_copy(&macro_pl->content, val);

	for (unsigned int i = 1; i < args->size - 1; i++ ) {
		cstring_t *arg = (cstring_t*)vector_push_blank(&macro_pl->arguments);
		cstring_init_copy(arg, (cstring_t*)vector_get(args, i));
	}

	token_def_t *def = token_def_create(token_macro, (void*)macro_pl);
	token_setdef(token, def);

	return NULL;
}

void *builtin_include(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 1 ) {
		fprintf(stderr, "missing arguments for builtin include\n");
		return NULL;
	}

	cstring_t *path = (cstring_t*)vector_get(args, 0);
	
	FILE *fp = NULL;

	for ( unsigned int i = include_directories.size; i > 0; i-- ) {
		cstring_t *dir = (cstring_t*)vector_get(&include_directories, i - 1);

		char *fullpath = alloca(sizeof(char) * (dir->size + path->size + 1));
		fullpath[0] = '\0';
	
		strcat(fullpath, dir->data);
		strcat(fullpath, path->data);
		fp = fopen(fullpath, "r");
		if ( fp != NULL ) {
			if ( debug ) {
				fprintf(stderr, "found file at %s\n", dir->data);
			}
			break;
		}
	}

	if ( fp == NULL ) {
		fprintf(stderr, "cannot open file \"%s\" for inclusion\n", path->data);
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

	cstring_t *name = (cstring_t*)vector_get(args, 0);

	token_t *token = get_token(store, name);


	if ( token && token->def ) {
		io_interface_t sub_io = *io;
		sub_io.get = io_get_string;
		sub_io.unget = io_unget_string;
		sub_io.eof = io_eof_string;
		
		cstring_t *result = (cstring_t*)vector_get(args, 1);

		char *string_reader = result->data;

		sub_io.in_payload = (void*)&string_reader;

		sub_base(&sub_io);
	} else if ( args->size > 2 ) { 
		io_interface_t sub_io = *io;
		sub_io.get = io_get_string;
		sub_io.unget = io_unget_string;
		sub_io.eof = io_eof_string;
		
		cstring_t *result = (cstring_t*)vector_get(args, 2);

		char *string_reader = result->data;

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

	cstring_t *name = (cstring_t*)vector_get(args, 0);

	token_t *token = get_token(store, name);


	if ( !token || !token->def ) {
		io_interface_t sub_io = *io;
		sub_io.get = io_get_string;
		sub_io.unget = io_unget_string;
		sub_io.eof = io_eof_string;
		
		cstring_t *result = (cstring_t*)vector_get(args, 1);

		char *string_reader = result->data;

		sub_io.in_payload = (void*)&string_reader;

		sub_base(&sub_io);
	} else if ( args->size > 2 ) { 
		io_interface_t sub_io = *io;
		sub_io.get = io_get_string;
		sub_io.unget = io_unget_string;
		sub_io.eof = io_eof_string;
		
		cstring_t *result = (cstring_t*)vector_get(args, 2);

		char *string_reader = result->data;

		sub_io.in_payload = (void*)&string_reader;

		sub_base(&sub_io);

	}

	return NULL;
}

void *builtin_ifeq(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 3 ) {
		fprintf(stderr, "missing arguments for builtin ifeq\n");
		return NULL;
	}

	cstring_t *wstr1 = (cstring_t*)vector_get(args, 0);
	cstring_t *wstr2 = (cstring_t*)vector_get(args, 1);


	if ( cstring_equal(wstr1, wstr2) ) {
		io_interface_t sub_io = *io;
		sub_io.get = io_get_string;
		sub_io.unget = io_unget_string;
		sub_io.eof = io_eof_string;
		
		cstring_t *result = (cstring_t*)vector_get(args, 2);

		char *string_reader = result->data;

		sub_io.in_payload = (void*)&string_reader;

		sub_base(&sub_io);
	} else if ( args->size > 3 ) { 
		io_interface_t sub_io = *io;
		sub_io.get = io_get_string;
		sub_io.unget = io_unget_string;
		sub_io.eof = io_eof_string;
		
		cstring_t *result = (cstring_t*)vector_get(args, 3);

		char *string_reader = result->data;

		sub_io.in_payload = (void*)&string_reader;

		sub_base(&sub_io);

	}

	return NULL;
}

void *builtin_ifneq(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 3 ) {
		fprintf(stderr, "missing arguments for builtin ifeq\n");
		return NULL;
	}

	cstring_t *wstr1 = (cstring_t*)vector_get(args, 0);
	cstring_t *wstr2 = (cstring_t*)vector_get(args, 1);


	if ( !cstring_equal(wstr1, wstr2) ) {
		io_interface_t sub_io = *io;
		sub_io.get = io_get_string;
		sub_io.unget = io_unget_string;
		sub_io.eof = io_eof_string;
		
		cstring_t *result = (cstring_t*)vector_get(args, 2);

		char *string_reader = result->data;

		sub_io.in_payload = (void*)&string_reader;

		sub_base(&sub_io);
	} else if ( args->size > 3 ) { 
		io_interface_t sub_io = *io;
		sub_io.get = io_get_string;
		sub_io.unget = io_unget_string;
		sub_io.eof = io_eof_string;
		
		cstring_t *result = (cstring_t*)vector_get(args, 3);

		char *string_reader = result->data;

		sub_io.in_payload = (void*)&string_reader;

		sub_base(&sub_io);

	}

	return NULL;
}

void *builtin_sys(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 1 ) {
		fprintf(stderr, "missing arguments for builtin sys\n");
		return NULL;
	}

	cstring_t *command = (cstring_t*)vector_get(args, 0);
	
	if ( debug ) {
		fprintf(stderr, "DEBUG: executing command \"%s\"\n", command->data);
	}

	FILE *output = popen(command->data, "r");
	
	if ( output == NULL ) {
		fprintf(stderr, "failed to execute command \"%s\"\n", command->data);
		return NULL;
	}
	
	cstring_t output_s;
	cstring_init_blank(&output_s, 30);

	while ( 1 ) {
		int c = fgetc(output);
		if ( c == EOF ) {
			cstring_put(&output_s, '\0');
			break;
		}
		cstring_put(&output_s, c);
	}

	pclose(output);

	io_interface_t sub_io = *io;

	char *string_reader = output_s.data;

	sub_io.get = io_get_string;
	sub_io.eof = io_eof_string;
	sub_io.unget = io_unget_string;
	sub_io.in_payload = (void*)&string_reader;

	sub_base(&sub_io);
	
	cstring_destroy(&output_s);

	return NULL;
}

void *builtin_mute(io_interface_t *io, token_store_t *store, vector_t *args) {
	mute++;
	return NULL;
}

void *builtin_unmute(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( mute > 0 ) {
		mute--;
	}

	return NULL;
}

void *builtin_log(io_interface_t *io, token_store_t *store, vector_t *args) {
	fprintf(stderr, "log: ");
	
	for ( unsigned int i = 0; i < args->size; i++ ) {
		cstring_t *arg = (cstring_t*)vector_get(args, i);
		fprintf(stderr, "%s ", arg->data);
	}

	fprintf(stderr, "\n");

	return NULL;
}



void setup_builtins(token_store_t *store) {
	{
		cstring_t key;
		cstring_init(&key, "pushs");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_pushs);
		token_pushdef(macro, def);
		cstring_destroy(&key);
	}

	{
		cstring_t key;
		cstring_init(&key, "pop");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_pop);
		token_pushdef(macro, def);
		cstring_destroy(&key);
	}
	
	{
		cstring_t key;
		cstring_init(&key, "pushm");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_pushm);
		token_pushdef(macro, def);
		cstring_destroy(&key);
	}

	{
		cstring_t key;
		cstring_init(&key, "setm");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_setm);
		token_pushdef(macro, def);
		cstring_destroy(&key);
	}
	
	{
		cstring_t key;
		cstring_init(&key, "sets");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_sets);
		token_pushdef(macro, def);
		cstring_destroy(&key);
	}
	
	{
		cstring_t key;
		cstring_init(&key, "set");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_set);
		token_pushdef(macro, def);
		cstring_destroy(&key);
	}

	{
		cstring_t key;
		cstring_init(&key, "unset");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_popall);
		token_pushdef(macro, def);
		cstring_destroy(&key);
	}
	
	{
		cstring_t key;
		cstring_init(&key, "include");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_include);
		token_pushdef(macro, def);
		cstring_destroy(&key);
	}

	{
		cstring_t key;
		cstring_init(&key, "ifset");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_ifdef);
		token_pushdef(macro, def);
		cstring_destroy(&key);
	}

	{
		cstring_t key;
		cstring_init(&key, "ifnset");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_ifndef);
		token_pushdef(macro, def);
		cstring_destroy(&key);
	}
	
	{
		cstring_t key;
		cstring_init(&key, "ifeq");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_ifeq);
		token_pushdef(macro, def);
		cstring_destroy(&key);
	}
	
	{
		cstring_t key;
		cstring_init(&key, "ifneq");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_ifneq);
		token_pushdef(macro, def);
		cstring_destroy(&key);
	}
	
	{
		cstring_t key;
		cstring_init(&key, "mute");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_mute);
		token_pushdef(macro, def);
		cstring_destroy(&key);
	}
	
	{
		cstring_t key;
		cstring_init(&key, "unmute");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_unmute);
		token_pushdef(macro, def);
		cstring_destroy(&key);
	}

	{
		cstring_t key;
		cstring_init(&key, "sys");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_sys);
		token_pushdef(macro, def);
		cstring_destroy(&key);
	}

	{
		cstring_t key;
		cstring_init(&key, "log");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_log);
		token_pushdef(macro, def);
		cstring_destroy(&key);
	}
	
	{
		cstring_t key;
		cstring_init(&key, "dnl");
		token_t *token = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_dnl, NULL);
		token_pushdef(token, def);
		cstring_destroy(&key);
	}




}
