# simple http server

# output file name
out = httpserver.exe
# gcc name
cc = gcc

all: main.o sock.o
	$(cc) -o $(out) main.o sock.o

main.o: main.c
	$(cc) -c main.c
	
sock.o: sock.c
	$(cc) -c sock.c
	
clean:
	del sock.o main.o $(out)
