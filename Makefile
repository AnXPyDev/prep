CC=gcc
BINDEPS=src/main.c src/token.h src/decl.h src/macro.h src/builtin.h src/io.h

main: prep

deps:
	[ ! -d libec ] && git clone https://github.com/anxpydev/libec.git --depth 1 && make -C libec || echo "libec installed"

prep: $(BINDEPS)
	$(CC) -Wall -O3 -I libec $(FLAGS) src/main.c -o prep

install: prep
	cp ./prep /usr/bin/prep
