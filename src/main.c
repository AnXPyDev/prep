#include <stdio.h>
#include <locale.h>
#include <wchar.h>
#include <malloc.h>

#include "decl.h"
#include "vector.h"
#include "wstring.h"
#include "hashmap.h"
#include "io.h"

FILE *infile;
FILE *outfile;

const wchar_t escape_wc = L'\\';
const wchar_t quote_open = L'`';
const wchar_t quote_close = L'\'';
const wchar_t quote_break = L'#';
const wchar_t quote_force_break = L'$';

wstring_t inclusive_chars;
wstring_t wc_buffer;
wstring_t quote_buffer;

vector_t include_directories;

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
	wchar_t wc;
	int eof = 0;
	int escaped = 0;
	unsigned int quoted = 0;
	int legible_token = 1;
	while ( 1 ) {
		eof = io_eof(io);
		
		if ( !eof ) {
			wc = io_get(io);
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

				if ( quoted == 1 ) {
					legible_token = 0;
				}

				// Handle token_string
				if ( token->def->type == token_string ) {
					wstring_t *string = (wstring_t*)token->def->payload;


					wchar_t *string_reader = string->data;
					
					io_interface_t sub_io = *io;

					sub_io.in_payload = (void*)&string_reader;
					sub_io.get = io_get_string;
					sub_io.eof = io_eof_string;
					sub_io.unget = io_unget_string;

					sub_base(&sub_io);
				} else if ( token->def->type == token_macro || token->def->type == token_builtin ) {
					io_unget(io, wc);
					sub_macro(io, token);
					continue;
				}
			} else {
				wstring_write(&wc_buffer, io->put, io->out_payload);
			}
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
			if ( escaped ) {
				goto write_wc;
			} else {
				escaped = 1;
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
				} else {
					legible_token = 0;
					goto write_wc;
				}
			} else if ( wc == quote_force_break || (quoted == 1 && wc == quote_break) ) {
				legible_token = 1;
			} else {
				goto write_wc;
			}
		} else {
			write_wc:
			escaped = 0;
			io_put(io, wc);
		}
	}

	return NULL;
}

int main(int argc, char **argv) {

	setlocale(LC_ALL, "");

	infile = stdin;
	outfile = stdout;
	wstring_init(&inclusive_chars, L"_-");
	wstring_init_blank(&wc_buffer, 128);
	wstring_init_blank(&quote_buffer, 128);

	vector_init(&include_directories, 1, sizeof(wstring_t));
	wstring_init((wstring_t*)vector_push_blank(&include_directories), L"./");
	
	token_store_init(&store);

	setup_builtins(&store);

	io_interface_t default_io = io_interface_ftf(stdin, stdout);

	sub_base(&default_io);

	token_store_destroy(&store);

	wstring_destroy(&inclusive_chars);
	wstring_destroy(&wc_buffer);
	wstring_destroy(&quote_buffer);
	
	return 0;
}
