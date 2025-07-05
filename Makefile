main: clean utils.o server.o client.o tcp.o io.o server_state_machine.o client_state_machine.o
	gcc main.c server.o client.o utils.o tcp.o io.o server_state_machine.o client_state_machine.o -I. -o tappy

server.o:
	gcc -c server.c -I. -o server.o

client.o:
	gcc -c client.c -I. -o client.o

tcp.o:
	gcc -c tcp.c -I. -o tcp.o

utils.o:
	gcc -c utils.c -I. -o utils.o

io.o:
	gcc -c io.c -I. -o io.o

client_state_machine.o: 
	gcc -c client_state_machine.c -I. -o client_state_machine.o

server_state_machine.o:
	gcc -c server_state_machine.c -I. -o server_state_machine.o

clean:
	rm -f *.o tappy