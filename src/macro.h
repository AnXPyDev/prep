#include <alloca.h>

typedef struct macro_payload {
	vector_t arguments;
	wstring_t content;
} macro_payload_t;

macro_payload_t *macro_create_payload() {
	macro_payload_t *result = malloc(sizeof(macro_payload_t));
	vector_init(&result->arguments, 2, sizeof(wstring_t));
	return result;
}

int macro_destroy_payload(macro_payload_t *macro) {
	
	for ( unsigned int i = 0; i < macro->arguments.size; i++ ) {
		wstring_destroy((wstring_t*)vector_get(&macro->arguments, i));
	}

	vector_destroy(&macro->arguments);
	wstring_destroy(&macro->content);

	return 0;
}

wchar_t macro_argument_list_open = L'(';
wchar_t macro_argument_list_close = L')';
wchar_t macro_argument_list_delimiter = L',';

void *sub_base(io_interface_t*);
typedef void *(*builtin_fn)(io_interface_t*, token_store_t*, vector_t*);

void *sub_macro(io_interface_t *io, token_t *macro_token) {

	macro_payload_t *macro = NULL;
	builtin_fn builtin = NULL;
	unsigned int argument_count = 2;

	if ( macro_token->def->type == token_macro ) {
		macro = (macro_payload_t*)macro_token->def->payload;
		argument_count = macro->arguments.size;
	} else if ( macro_token->def->type == token_builtin ) {
		builtin = (builtin_fn)macro_token->def->payload;
	}

	vector_t arguments;
	vector_init(&arguments, argument_count, sizeof(wstring_t));

	wstring_t *argument = vector_push_blank(&arguments);
	wstring_init_blank(argument, 20);
	
	unsigned int aix = 0;
	
	wchar_t wc;
	int eof = 0;
	int escaped = 0;
	unsigned int quoted = 0;
	int opened = 0;
	unsigned int whitespace = 0;

	while ( 1 ) {
		eof = io_eof(io);
		if ( !eof ) {
			wc = io_get(io);
		} else {
			fprintf(stderr, "EOF while reading argument list for macro %ls\n", macro_token->name.data);
			break;
		}

		// Handle escaped characters
		if ( wc == escape_wc ) {
			if ( !escaped ) {
				escaped = 1;
			}
		//Handle quotes
		} else if ( !escaped ) {
			if ( wc == quote_open ) {
				quoted++;
			} else if ( wc == quote_close ) {
				if ( quoted != 0 ) {
					quoted--;
				}
			}
		}

		if ( opened == 1 && wc != L' ' ) {
			opened = 2;
		}

		if ( !opened && !quoted && wc == macro_argument_list_open ) {
			opened = 1;
		} else if ( opened && !quoted && wc == macro_argument_list_delimiter ) {
			wstring_strip_trailing(argument, L' ');
			aix++;
			argument = vector_push_blank(&arguments);
			wstring_init_blank(argument, 20);
			opened = 1;
		} else if ( opened == 2 && !quoted && wc == macro_argument_list_close ) {
			wstring_strip_trailing(argument, L' ');
			opened = 0;
			break;
		} else if ( opened == 2 ) {
			wstring_putwc(argument, wc);
		}

	}

	
	
	token_t **arg_tokens = (token_t**)alloca(sizeof(token_t*) * argument_count);

	vector_t arguments_processed;
	vector_init(&arguments_processed, arguments.size, sizeof(wstring_t));
	
	if ( builtin ) {
		for ( unsigned int i = 0; i < arguments.size; i++ ) {
			wstring_t *arg_val = (wstring_t*)vector_get(&arguments, i);
			wstring_t *processed = (wstring_t*)vector_push_blank(&arguments_processed);
			wstring_init_blank(processed, 20);
			wchar_t *string_reader = arg_val->data;
			io_interface_t sub_io = io_interface_sts(&string_reader, processed);
			sub_base(&sub_io);
		}

		builtin(io, &store, &arguments_processed);

		return NULL;
	}

	if ( !macro ) {
		return NULL;
	}

	for ( unsigned int i = 0; i < argument_count; i++ ) {
		wstring_t *arg_name = (wstring_t*)vector_get(&macro->arguments, i);
		wstring_t *arg_val = (wstring_t*)vector_get(&arguments, i);
		

		wstring_t *processed = (wstring_t*)vector_push_blank(&arguments_processed);
		wstring_init_blank(processed, 20);
		
		
		wchar_t *string_reader = arg_val->data;
		
		io_interface_t sub_io = io_interface_sts(&string_reader, processed);

		sub_base(&sub_io);

		token_def_t *arg_token_def = token_def_create(token_string, (void*)processed);
		token_t *arg_token = ensure_token(&store, arg_name);
		arg_tokens[i] = arg_token;
	
		token_pushdef(arg_token, arg_token_def);
	}

	wchar_t *string_reader = macro->content.data;

	
	io_interface_t next_io = *io;
	next_io.in_payload = &string_reader;
	next_io.get = io_get_string;
	next_io.eof = io_eof_string;
	next_io.unget = io_unget_string;

	sub_base(&next_io);

	for ( unsigned int i = 0; i < argument_count; i++ ) {
		token_t *token = arg_tokens[i];
		token->def->type = token_null;
		token->def->payload = NULL;
		token_popdef(token);
		wstring_destroy((wstring_t*)vector_get(&arguments, i));
	}

	vector_destroy(&arguments);

	
	return NULL;
}