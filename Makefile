all: main

main: main.c utility.h
	@gcc main.c -o main

.PHONY: clean

clean:
	rm -f main

