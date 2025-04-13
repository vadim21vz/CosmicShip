CFLAGS = -Wall -Wextra
LDFLAGS = -lm

game: main.o
	gcc $(CFLAGS) -o game main.o $(LDFLAGS)

main.o: main.c
	gcc $(CFLAGS) -c main.c

clean:
	rm -f game main.o