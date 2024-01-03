CC = clang
CFLAGS = -Wall -Wpedantic -Werror -Wextra

all: httpserver

httpserver: httpserver.o asgn4_helper_funcs.a
	$(CC) -o httpserver httpserver.o asgn4_helper_funcs.a

httpserver.o: httpserver.c
	$(CC) $(CFLAGS) -c httpserver.c

clean:
	rm -f httpserver *.o *.pdf

format:
	clang-format -i -style=file httpserver.c
