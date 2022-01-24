CC=gcc
BINDEPS=src/main.c src/vector.h src/token.h src/wstring.h src/hashmap.h src/decl.h src/macro.h src/builtin.h src/io.h

main: prep

prep: $(BINDEPS)
	$(CC) -Wall -O3 $(FLAGS) src/main.c -o prep

install: prep
	cp ./prep /usr/bin/prep
