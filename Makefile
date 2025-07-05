main: clean utils.o server.o client.o tcp.o io.o
	gcc main.c server.o client.o utils.o tcp.o io.o -I. -o tappy

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

clean:
	rm -f *.o tappy