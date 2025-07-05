main: clean utils.o server.o client.o tcp.o
	gcc main.c server.o client.o utils.o tcp.o -I. -o tappy

server.o:
	gcc -c server.c -I. -o server.o

client.o:
	gcc -c client.c -I. -o client.o

tcp.o:
	gcc -c tcp.c -I. -o tcp.o

utils.o:
	gcc -c utils.c -I. -o utils.o

clean:
	rm -f *.o tappy