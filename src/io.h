typedef char (*io_get_fn)(void*);
typedef void (*io_put_fn)(char, void*);
typedef int (*io_eof_fn)(void*);
typedef void (*io_unget_fn)(char, void*);

typedef struct io_interface_s {
	 io_get_fn get;
	 io_put_fn put;
	 io_eof_fn eof;
	 io_unget_fn unget;

	 void *in_payload;
	 void *out_payload;
	 int is_default_out;
} io_interface_t;



char io_get_file(void *file) {
	return fgetc((FILE*)file);
}

void io_put_file(char c, void *file) {
	fputc(c, (FILE*)file);
}

int io_eof_file(void *file) {
	return feof((FILE*)file);
}

void io_unget_file(char c, void *file) {
	ungetc(c, (FILE*)file);
}

char io_get_string(void *string) {
	char result = *(*(char**)string);
	(*(char**)string)++;
	return result;
}

void io_put_string(char c, void *string) {
	cstring_put((cstring_t*)string, c);
}

int io_eof_string(void *string) {
	return **(char**)string == '\0';
}

void io_unget_string(char c, void *string) {
	(*(char**)string)--;
}

char io_get(io_interface_t *io) {
	return io->get(io->in_payload);
}

#include <signal.h>

void io_put(io_interface_t *io, char c) {
	if ( (io->is_default_out && !mute) || !io->is_default_out ) {
		io->put(c, io->out_payload);
	}
}

int io_eof(io_interface_t *io) {
	return io->eof(io->in_payload);
}

void io_unget(io_interface_t *io, char c) {
	return io->unget(c, io->in_payload);
}

io_interface_t io_interface_ftf(FILE *infile, FILE *outfile) {
	io_interface_t io;

	io.get = io_get_file;
	io.put = io_put_file;
	io.eof = io_eof_file;
	io.unget = io_unget_file;

	io.in_payload = (void*)infile;
	io.out_payload = (void*)outfile;
	io.is_default_out = 0;

	return io;
}

io_interface_t io_interface_sts(char **instring_reader, cstring_t *outstring) {
	io_interface_t io;

	io.get = io_get_string;
	io.put = io_put_string;
	io.eof = io_eof_string;
	io.unget = io_unget_string;

	io.in_payload = (void*)instring_reader;
	io.out_payload = (void*)outstring;
	io.is_default_out = 0;

	return io;
}

int cstring_write(const cstring_t *cstring, io_interface_t *io) {
	for ( unsigned int i = 0; i < cstring->size; i++ ) {
		io_put(io, cstring->data[i]);
	}
	return 0;
}
