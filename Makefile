CC = gcc
FLAGS = -g -Wall 
OBJ = tarea2.o tablero.o

all: juego
juego: $(OBJ)
	$(CC) -o juego tarea2.o

tarea2.o: tarea2.c
	$(CC) $(FLAGS) -c tarea2.c
tablero.o: tablero.h tablero.c 
	$(CC) $(FLAGS) -c tablero.c

clean:
	rm $(OBJ) juego 