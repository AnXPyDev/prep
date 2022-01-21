#include <stdio.h>
#include <locale.h>
#include <wchar.h>
#include <malloc.h>
#include <stdlib.h>

#include "decl.h"
#include "vector.h"
#include "wstring.h"
#include "hashmap.h"

int mute = 0;

#include "io.h"

int debug = 0;

FILE *outfile;


#ifndef PREP_CONFIG_INCLUDED
	#include "config.h"
#endif

wstring_t inclusive_chars;
wstring_t wc_buffer;
wstring_t quote_buffer;
vector_t include_directories;

struct input_file {
	FILE *file;
	int mute;
};

vector_t input_files;

#include "token.h"

token_store_t store;

#include "macro.h"
#include "builtin.h"

int check_inclusive_wc(wchar_t wc) {
	if ( ((store.wc_count != 0 && (wc >= 48 && wc <= 57)) || (wc >= 65 && wc <= 90) || (wc >= 97 && wc <= 122)) ) {
		return 1;
	}

       	return wstring_contains_wc(&inclusive_chars, wc);
}


void *sub_base(io_interface_t *io) {
	wstring_t log;
	wstring_init_blank(&log, 12);
	wchar_t wc;
	int eof = 0;
	int escaped = 0;
	unsigned int quoted = 0;
	int legible_token = 1;
	int dnl = 0;
	int broken = 0;

	while ( 1 ) {
		eof = io_eof(io);
	
		if ( !eof ) {
			wc = io_get(io);
			if ( wc == -1 ) { 
				eof = 1;
			} else {
				wstring_putwc(&log, wc);
			}
		}
		
		
		if ( dnl ) {
			if ( wc == L'\n' ) {
				dnl = 0;
			}
			continue;
		}

		if ( !eof && !escaped && legible_token && check_inclusive_wc(wc) ) {
			wstring_putwc(&wc_buffer, wc);
			match_token_wc(&store, wc);
			continue;
		} else if ( store.wc_count != 0 ) {
			token_t *token = get_token_match(&store);
			if ( token != NULL ) {
				wstring_set_size(&wc_buffer, 0);
				reset_token_match(&store);
			}
			if ( token != NULL && token->def != NULL ) {
				broken = 0;
				if ( quoted ) {
					legible_token = 0;
				}

				// Handle token_string
				if ( token->def->type == token_string ) {
					wstring_t *string = (wstring_t*)token->def->payload;
					//fprintf(stderr, "Sub string: {%ls}\n", string->data);

					wchar_t *string_reader = string->data;
					
					io_interface_t sub_io = *io;

					sub_io.in_payload = (void*)&string_reader;
					sub_io.get = io_get_string;
					sub_io.eof = io_eof_string;
					sub_io.unget = io_unget_string;

					sub_base(&sub_io);
				} else if ( token->def->type == token_macro || token->def->type == token_builtin ) {
					//fprintf(stderr, "Sub macro %ls\n", token->name.data);
					io_unget(io, wc);
					sub_macro(io, token);
					continue;
				} else if ( token->def->type == token_dnl ) {
					dnl = 1;
					io_unget(io, wc);
					continue;
				}
			} else {
				if ( broken == 1 ) {
					io_put(io, quote_break);
				} else if ( broken == 2 ) {
					io_put(io, quote_force_break);
				}
				broken = 0;
				wstring_write(&wc_buffer, io);
			}
		} else {
			if ( broken == 1 ) {
				io_put(io, quote_break);
			} else if ( broken == 2 ) {
				io_put(io, quote_force_break);
			}
			broken = 0;
		}
	
		if ( wc_buffer.size != 0 ) {
			wstring_set_size(&wc_buffer, 0);
			reset_token_match(&store);
		}
		
		if ( eof ) {
			break;
		}
		
	
		// Handle escaped characters
		if ( wc == escape_wc ) {
			if ( !escaped ) {
				if ( quoted ) {
					escaped = 2;
					goto write_wc;
				} else {
					escaped = 1;
				}
			} else {
				goto write_wc;
			}
		//Handle quotes
		} else if ( !escaped ) {
			if ( wc == quote_open ) {
				quoted++;
				legible_token = 0;

				if ( quoted > 1 ) {
					goto write_wc;
				}
			} else if ( wc == quote_close ) {
				if ( quoted != 0 ) {
					quoted--;
					if ( quoted != 0 ) {
						goto write_wc;
					}
					legible_token = 1;
				} else {
					goto write_wc;
				}
			} else if ( wc == quote_force_break ) {
				legible_token = 1;
				broken = 2;
			} else if ( quoted <= 1 && wc == quote_break ) {
				legible_token = 1;
				broken = 1;
			} else if ( quoted == 1 && wc == quote_reset ) {
				legible_token = 1;
			} else {
				goto write_wc;
			}
		} else {
			write_wc:
			if ( escaped ) {
				escaped--;
			}
			io_put(io, wc);
		}
	}
	

	wstring_destroy(&log);

	return NULL;
}

void add_include_dir(wchar_t *path) {
	wstring_t *string = (wstring_t*)vector_push_blank(&include_directories);
	wstring_init(string, path);
	if ( string->data[string->size - 1] != L'/' ) {
		wstring_putwc(string, L'/');
	}
}

int process_args(int argc, char **argv) {
	int mute = 0;
	char *argname = "";
	for ( int i = 0; i < argc; i++ ) {
		char *arg = argv[i];
		if ( arg[0] == '-' ) {
			if ( strcmp(arg, "--debug") == 0 ) {
				debug = 1;
			} else if ( strcmp(arg, "--mute") == 0 || strcmp(arg, "-m") == 0 ) {
				mute = 1;
			} else if ( strcmp(arg, "--stdin") == 0 || strcmp(arg, "-0") == 0 ) {
				struct input_file *inf = (struct input_file*)vector_push_blank(&input_files);
				inf->file = stdin;
				inf->mute = mute;
				mute = 0;
				
				if ( debug ) {
					fprintf(stderr, "DBG: adding stdin to input files\n");
				}

			} else {
				argname = arg;
			}
		} else {
			if ( strcmp(argname, "-I") == 0 ) {
				wchar_t path[1024];
				mbstowcs(path, arg, 1024);
	
				if ( debug ) {
					fprintf(stderr, "DBG: adding directory %ls to include directories\n", path);
				}

				add_include_dir(path);
			} else if ( strcmp(argname, "-D") == 0 ) {
				wchar_t name_buf[1024];

				wstring_t name;
				name.capacity = 1024;
				name.data = name_buf;
				name.size = 0;

				wstring_t *val = NULL;
				
				char *cptr = arg;

				while ( *cptr && *cptr != L'=' ) {
					cptr++;
				}

				if ( *cptr == L'=' ) {
					*cptr = L'\0';

					val = malloc(sizeof(wstring_t));
					wstring_init_blank(val, 1024);

					mbstowcs(val->data, cptr + 1, 1024);
					wstring_reload_size(val);
				}
				
				mbstowcs(name_buf, arg, 1024);
				wstring_reload_size(&name);

				token_t *token = ensure_token(&store, &name);

				token_def_t *def;

				if ( val ) {
					def = token_def_create(token_string, val);
				} else {
					def = token_def_create(token_empty, NULL);
				}

				token_pushdef(token, def);
				

			} else if ( strcmp(argname, "--input") == 0 || strcmp(argname, "-i") == 0 ) {
				if ( debug ) {
					fprintf(stderr, "DBG: adding file \"%s\" as input\n", arg);
				}
				FILE *fp = fopen(arg, "r");
				if ( fp == NULL ) {
					fprintf(stderr, "cannot open file \"%s\" for input\n", arg);
				} else {
					if ( debug ) {
						fprintf(stderr, "DBG: input file \"%s\" opened successfully\n", arg);
					}
					struct input_file *inf = (struct input_file*)vector_push_blank(&input_files);
					inf->file = fp;
					inf->mute = mute;
				}
				mute = 0;
			} else if ( strcmp(argname, "--output") == 0 || strcmp(argname, "-o") == 0 ) {
				if ( debug ) {
					fprintf(stderr, "DBG: opening file \"%s\" as output\n", arg);
				}
				FILE *fp = fopen(arg, "w");
				if ( fp == NULL ) {
					fprintf(stderr, "cannot open file \"%s\" as output, defaulting to stdout\n", arg);
				} else {
					if ( debug ) {
						fprintf(stderr, "DBG: output file \"%s\" opened successfully\n", arg);
					}
					outfile = fp;
				}
			}
		}
	}
	return 0;
}

int main(int argc, char **argv) {

	if ( argc > 1 && strcmp(argv[1], "--debug") == 0 ) {
		debug = 1;
	}

	setlocale(LC_ALL, "");

	outfile = stdout;
	wstring_init(&inclusive_chars, token_inclusive_chars);
	wstring_init_blank(&wc_buffer, 128);
	wstring_init_blank(&quote_buffer, 128);

	vector_init(&include_directories, 1, sizeof(wstring_t));
	wstring_init((wstring_t*)vector_push_blank(&include_directories), L"./");

	vector_init(&input_files, 1, sizeof(struct input_file));
	
	token_store_init(&store);

	setup_builtins(&store);

	if ( process_args(argc, argv) == 1 ) {
		return 1;
	}

	if ( input_files.size == 0 ) {
		struct input_file *inf = (struct input_file*)vector_push_blank(&input_files);
		inf->file = stdin;
		inf->mute = 0;
	}

	for ( unsigned int i = 0; i < input_files.size; i++ ) {
		struct input_file *inf = (struct input_file*)vector_get(&input_files, i);
		mute = inf->mute;	

		io_interface_t default_io = io_interface_ftf(inf->file, outfile);
		default_io.is_default_out = 1;

		sub_base(&default_io);
	}


	token_store_destroy(&store);

	wstring_destroy(&inclusive_chars);
	wstring_destroy(&wc_buffer);
	wstring_destroy(&quote_buffer);

	for ( unsigned int i = 0; i < include_directories.size; i++ ) {
		wstring_destroy((wstring_t*)vector_get(&include_directories, i));
	}

	vector_destroy(&include_directories);
	
	for ( unsigned int i = 0; i < input_files.size; i++ ) {
		struct input_file *inf = (struct input_file*)vector_get(&input_files, i);
		if ( inf->file != stdin ) {
			fclose(inf->file);
		}
	}

	vector_destroy(&input_files);

	if ( outfile != stdout ) {
		fclose(outfile);
	}
	
	return 0;
}
