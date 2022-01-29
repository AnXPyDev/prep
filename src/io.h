typedef wchar_t (*io_get_fn)(void*);
typedef void (*io_put_fn)(wchar_t, void*);
typedef int (*io_eof_fn)(void*);
typedef void (*io_unget_fn)(wchar_t, void*);

typedef struct io_interface_s {
	 io_get_fn get;
	 io_put_fn put;
	 io_eof_fn eof;
	 io_unget_fn unget;

	 void *in_payload;
	 void *out_payload;
	 int is_default_out;
} io_interface_t;



wchar_t io_get_file(void *file) {
	return fgetwc((FILE*)file);
}

void io_put_file(wchar_t wc, void *file) {
	fputwc(wc, (FILE*)file);
}

int io_eof_file(void *file) {
	return feof((FILE*)file);
}

void io_unget_file(wchar_t wc, void *file) {
	ungetwc(wc, (FILE*)file);
}

wchar_t io_get_string(void *string) {
	wchar_t result = *(*(wchar_t**)string);
	(*(wchar_t**)string)++;
	return result;
}

void io_put_string(wchar_t wc, void *string) {
	wstring_putwc((wstring_t*)string, wc);
}

int io_eof_string(void *string) {
	return **(wchar_t**)string == L'\0';
}

void io_unget_string(wchar_t wc, void *string) {
	(*(wchar_t**)string)--;
}

wchar_t io_get(io_interface_t *io) {
	return io->get(io->in_payload);
}

#include <signal.h>

void io_put(io_interface_t *io, wchar_t wc) {
	if ( (io->is_default_out && !mute) || !io->is_default_out ) {
		io->put(wc, io->out_payload);
	}
}

int io_eof(io_interface_t *io) {
	return io->eof(io->in_payload);
}

void io_unget(io_interface_t *io, wchar_t wc) {
	return io->unget(wc, io->in_payload);
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

io_interface_t io_interface_sts(wchar_t **instring_reader, wstring_t *outstring) {
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

int wstring_write(const wstring_t *wstring, io_interface_t *io) {
	for ( unsigned int i = 0; i < wstring->size; i++ ) {
		io_put(io, wstring->data[i]);
	}
	return 0;
}
