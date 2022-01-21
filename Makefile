CC=gcc
BINDEPS=src/main.c src/vector.h src/token.h src/wstring.h src/hashmap.h src/decl.h src/macro.h src/builtin.h src/io.h

main: prep

debug: $(BINDEPS)
	$(CC) -Wall -O3 -g src/main.c -o prep-debug

prep: $(BINDEPS)
	$(CC) -Wall -O3 src/main.c -o prep

install: prep
	cp ./prep /usr/bin/prep

install-debug: debug
	cp ./prep-debug /usr/bin/prep
