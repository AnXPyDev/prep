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

void *builtin_sets(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 2 ) {
		fprintf(stderr, "missing arguments for builtin sets\n");
		return NULL;
	}

	wstring_t *name = (wstring_t*)vector_get(args, 0);
	wstring_t *val = (wstring_t*)vector_get(args, 1);

	token_t *token = ensure_token(store, name);
	wstring_t *content = malloc(sizeof(wstring_t));
	wstring_init_copy(content, val);
	token_def_t *def = token_def_create(token_string, (void*)content);
	token_setdef(token, def);

	return NULL;
}

void *builtin_set(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 1 ) {
		fprintf(stderr, "missing arguments for builtin setdef\n");
		return NULL;
	}

	wstring_t *name = (wstring_t*)vector_get(args, 0);
	wstring_t *val = (wstring_t*)vector_get(args, 1);

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

	wstring_t *name = (wstring_t*)vector_get(args, 0);

	token_t *token = ensure_token(store, name);
	token_popdef(token);

	return NULL;
}

void *builtin_popall(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 1 ) {
		fprintf(stderr, "missing arguments for builtin popall\n");
		return NULL;
	}

	wstring_t *name = (wstring_t*)vector_get(args, 0);

	token_t *token = ensure_token(store, name);
	token_undef(token);

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

void *builtin_setm(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 3 ) {
		fprintf(stderr, "missing arguments for builtin setm\n");
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
	token_setdef(token, def);

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

void *builtin_sys(io_interface_t *io, token_store_t *store, vector_t *args) {
	if ( args->size < 1 ) {
		fprintf(stderr, "missing arguments for builtin sys\n");
		return NULL;
	}

	wstring_t *command = (wstring_t*)vector_get(args, 0);
	
	char command_buffer[1024];

	wcstombs(command_buffer, command->data, 1024);

	if ( debug ) {
		fprintf(stderr, "DEBUG: executing command \"%s\"\n", command_buffer);
	}

	FILE *output = popen(command_buffer, "r");
	
	if ( output == NULL ) {
		fprintf(stderr, "failed to execute command \"%s\"\n", command_buffer);
		return NULL;
	}
	
	vector_t output_v;
	vector_init(&output_v, 30, sizeof(char));

	while ( 1 ) {
		int c = fgetc(output);
		if ( c == EOF ) {
			*(char*)vector_push_blank(&output_v) = '\0';
			break;
		}
		*(char*)vector_push_blank(&output_v) = c;
	}

	pclose(output);

	unsigned int wsize = mbstowcs(NULL, output_v.data, output_v.size + 1);

	wchar_t *wout = malloc(sizeof(wchar_t) * (wsize + 5));

	mbstowcs(wout, output_v.data, wsize + 4);

	io_interface_t sub_io = *io;

	wchar_t *string_reader = wout;

	sub_io.get = io_get_string;
	sub_io.eof = io_eof_string;
	sub_io.unget = io_unget_string;
	sub_io.in_payload = (void*)&string_reader;

	sub_base(&sub_io);

	vector_destroy(&output_v);
	free(wout);

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
		wstring_t *arg = (wstring_t*)vector_get(args, i);
		fprintf(stderr, "%ls ", arg->data);
	}

	fprintf(stderr, "\n");

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
		wstring_init(&key, L"setm");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_setm);
		token_pushdef(macro, def);
		wstring_destroy(&key);
	}
	
	{
		wstring_t key;
		wstring_init(&key, L"sets");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_sets);
		token_pushdef(macro, def);
		wstring_destroy(&key);
	}
	
	{
		wstring_t key;
		wstring_init(&key, L"set");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_set);
		token_pushdef(macro, def);
		wstring_destroy(&key);
	}

	{
		wstring_t key;
		wstring_init(&key, L"unset");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_popall);
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
		wstring_init(&key, L"ifset");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_ifdef);
		token_pushdef(macro, def);
		wstring_destroy(&key);
	}

	{
		wstring_t key;
		wstring_init(&key, L"ifnset");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_ifndef);
		token_pushdef(macro, def);
		wstring_destroy(&key);
	}
	
	{
		wstring_t key;
		wstring_init(&key, L"mute");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_mute);
		token_pushdef(macro, def);
		wstring_destroy(&key);
	}
	
	{
		wstring_t key;
		wstring_init(&key, L"unmute");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_unmute);
		token_pushdef(macro, def);
		wstring_destroy(&key);
	}

	{
		wstring_t key;
		wstring_init(&key, L"sys");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_sys);
		token_pushdef(macro, def);
		wstring_destroy(&key);
	}

	{
		wstring_t key;
		wstring_init(&key, L"log");
		token_t *macro = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_builtin, (void*)&builtin_log);
		token_pushdef(macro, def);
		wstring_destroy(&key);
	}
	
	{
		wstring_t key;
		wstring_init(&key, L"dnl");
		token_t *token = register_token_blank(store, &key);
		token_def_t *def = token_def_create(token_dnl, NULL);
		token_pushdef(token, def);
		wstring_destroy(&key);
	}




}
