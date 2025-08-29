CFLAGS = -Wall -Wextra -std=c99 -fsanitize=address -fno-omit-frame-pointer
INCLUDES = -I.

BOOGIE_INCLUDE = -I/usr/local/include
BOOGIE_STATIC_LIB = /usr/local/lib/libboogielib.a
INCLUDES += $(BOOGIE_INCLUDE)

LDFLAGS = $(BOOGIE_STATIC_LIB) -fsanitize=address -static-libgcc -lstdc++

main: clean utils.o server.o client.o tcp.o io.o server_state_machine.o client_state_machine.o
	gcc main.c server.o client.o utils.o tcp.o io.o server_state_machine.o client_state_machine.o $(INCLUDES) $(LDFLAGS) -o tappy

server.o:
	gcc -c server.c $(INCLUDES) $(CFLAGS) -o server.o

client.o:
	gcc -c client.c $(INCLUDES) $(CFLAGS) -o client.o

tcp.o:
	gcc -c tcp.c $(INCLUDES) $(CFLAGS) -o tcp.o

utils.o:
	gcc -c utils.c $(INCLUDES) $(CFLAGS) -o utils.o

io.o:
	gcc -c io.c $(INCLUDES) $(CFLAGS) -o io.o

client_state_machine.o: 
	gcc -c client_state_machine.c $(INCLUDES) $(CFLAGS) -o client_state_machine.o

server_state_machine.o:
	gcc -c server_state_machine.c $(INCLUDES) $(CFLAGS) -o server_state_machine.o

clean:
	rm -f *.o tappy

.PHONY: clean
