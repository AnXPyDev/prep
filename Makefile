CC=gcc

main: prep

prep: src/main.c src/vector.h src/token.h src/wstring.h src/hashmap.h src/decl.h src/macro.h src/builtin.h src/io.h
	$(CC) src/main.c -o prep -g

install: prep
	cp ./prep /usr/bin/prep
