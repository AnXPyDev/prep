typedef struct macro_payload {
	vector_t arguments;
	cstring_t content;
} macro_payload_t;

macro_payload_t *macro_create_payload() {
	macro_payload_t *result = malloc(sizeof(macro_payload_t));
	vector_init(&result->arguments, 2, sizeof(cstring_t));
	return result;
}

int macro_destroy_payload(macro_payload_t *macro) {
	
	for ( unsigned int i = 0; i < macro->arguments.size; i++ ) {
		cstring_destroy((cstring_t*)vector_get(&macro->arguments, i));
	}

	vector_destroy(&macro->arguments);
	cstring_destroy(&macro->content);

	return 0;
}

void *sub_base(io_interface_t*);
typedef void *(*builtin_fn)(io_interface_t*, token_store_t*, vector_t*);

void *sub_macro(io_interface_t *io, token_t *macro_token) {
	cstring_t log;
	cstring_init_blank(&log, 12);

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
	vector_init(&arguments, argument_count, sizeof(cstring_t));

	cstring_t *argument = NULL;
	unsigned int aix = 0;
	
	char c;
	int eof = 0;
	int escaped = 0;
	unsigned int quoted = 0;
	int opened = 0;

	while ( 1 ) {
		eof = io_eof(io);

		if ( !eof ) {
			c = io_get(io);
			if ( c == -1 ) {
				eof = 1;
			}
		}

		if ( eof ) {
			fprintf(stderr, "EOF while reading argument list for macro %s\n	at: %s\n", macro_token->name.data, log.data);
			return NULL;
			break;
		}

		cstring_put(&log, c);


		// Handle escaped characters
		if ( c == escape_c ) {
			if ( !escaped ) {
				escaped = 1;
			}
		//Handle quotes
		} else if ( !escaped ) {
			if ( c == quote_open ) {
				quoted++;
			} else if ( c == quote_close ) {
				if ( quoted != 0 ) {
					quoted--;
				}
			}
		} else {
			escaped = 0;

		}


		if ( opened == 1 && c != ' ' && c != macro_argument_list_close && c != macro_argument_list_delimiter ) {
			opened = 2;
			if ( !argument ) {
				argument = vector_push_blank(&arguments);
				cstring_init_blank(argument, 20);
			}
		}
	
		if ( !opened && c != ' ' && c != '\n' && c != macro_argument_list_open ) {
			fprintf(stderr, "no opening for argument list in macro %s\n", macro_token->name.data);
			return NULL;
		}

		if ( !opened && !quoted && c == macro_argument_list_open ) {
			opened = 1;
		} else if ( opened && !quoted && c == macro_argument_list_delimiter ) {
			if ( argument ) {
				cstring_strip_trailing(argument, ' ');
				aix++;
				argument = vector_push_blank(&arguments);
				cstring_init_blank(argument, 20);
				opened = 1;
			}
		} else if ( opened && !quoted && c == macro_argument_list_close ) {
			if ( argument ) {
				cstring_strip_trailing(argument, ' ');
			}
			opened = 0;
			break;
		} else if ( opened == 2 ) {
			cstring_put(argument, c);
		}

	}

	token_t **arg_tokens = (token_t**)alloca(sizeof(token_t*) * argument_count);

	vector_t arguments_processed;

	vector_init(&arguments_processed, arguments.size, sizeof(cstring_t));
		
	if ( builtin ) {
		for ( unsigned int i = 0; i < arguments.size; i++ ) {
			cstring_t *arg_val = (cstring_t*)vector_get(&arguments, i);
			cstring_t *processed = (cstring_t*)vector_push_blank(&arguments_processed);
			cstring_init_blank(processed, 20);
			char *string_reader = arg_val->data;
			io_interface_t sub_io = io_interface_sts(&string_reader, processed);
			sub_base(&sub_io);
		}

		builtin(io, &store, &arguments_processed);

		return NULL;
	}

	if ( !macro ) {
		return NULL;
	}

	if ( arguments.size < argument_count ) {
		fprintf(stderr, "not enough arguments provided to macro \"%s\"\n", macro_token->name.data);
	}

	for ( unsigned int i = 0; i < argument_count; i++ ) {
		cstring_t *arg_name = (cstring_t*)vector_get(&macro->arguments, i);
		cstring_t *arg_val = (cstring_t*)vector_get(&arguments, i);
		

		cstring_t *processed = (cstring_t*)vector_push_blank(&arguments_processed);
		cstring_init_blank(processed, 20);
		
		
		char *string_reader = arg_val->data;
		
		io_interface_t sub_io = io_interface_sts(&string_reader, processed);

		sub_base(&sub_io);

		token_def_t *arg_token_def = token_def_create(token_string, (void*)processed);
		token_t *arg_token = ensure_token(&store, arg_name);
		arg_tokens[i] = arg_token;
	
		token_pushdef(arg_token, arg_token_def);
	}

	char *string_reader = macro->content.data;

	
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
		cstring_destroy((cstring_t*)vector_get(&arguments, i));
	}

	vector_destroy(&arguments);

	cstring_destroy(&log);

	
	return NULL;
}
