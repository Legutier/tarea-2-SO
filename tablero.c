#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>    

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>

#include "tablero.h"


// Construye el tablero inicial
void init(struct tablero *a){
    // componentes de los casilleros
    int players[4];
    int signos;
    char eventos[6];
   	char flecha[4];

   	// componentes del tablero
   	int i, j;
   	int rows = 5;
   	int columns; 

    a->actual = NULL;
    a->head = NULL;
    a->tail = NULL;
    a->length = 0;


    for (i = 0; i < 4; i++)
    	players[i] = 0;

    for (i = 0; i < rows; i++){
    	if (i % 2 == 0)
    		columns = 9;
    	else
    		columns = 1;
    	for (j = 0; j < columns; j++){

    		// Asigna la flecha correspondiente a los casilleros
    		if ((i == 0 || i == 4) && j == 0)
    			strcpy(flecha, "→");
    		else if ((i == 0 || i == 2) && j == 8)
    			strcpy(flecha, "↑");
    		else if (i == 2 && j == 0)
    			strcpy(flecha, "←");
    		else
    			strcpy(flecha, " ");

    		// Asigna los eventos correspondientes a los casilleros
    		if (i == 0 && j == 0){
    			strcpy(eventos, "start");
    			signos = 0;
    		}
    		else if(i == 4 && j == 8){
    			strcpy(eventos, " fin ");
    			signos = 0;
    		}

    		else if((i == 0 && (j == 2 || j == 4 || j == 6)) || (i == 2 && (j == 2 || j == 4)) || (i == 4 && j % 2 == 1)){
    			strcpy(eventos, "  ?  ");
    			signos = 1;
    		}

    		else if((i == 2 && j == 6) || (i == 4 && (j == 2 || j == 4 || j == 6))){
    			strcpy(eventos, " ? ? ");
    			signos = 2;
    		}
    		else{
    			strcpy(eventos, "     ");
    			signos = 0;
    		}
    		// Agrega el casillero con los valores obtenidos
    		append(a, players, flecha, eventos, signos);
    	}
    } 

    // Agrega a los jugadores a la casilla start
    for (i = 0; i < 4; i++)
    	actualizarPosicion(a, i + 1, 0, 0);
    return;
}

// libera la memoria del tablero y de sus casilleros
void clear(struct tablero *a){
    int idCasillero;
    while(a->head != NULL){
        a->actual = a->head;
        a->head = a->head->next;
        idCasillero = a->actual->idMemoria;
        // libera la memoria del casillero
        shmdt ((char *)a->actual);
		shmctl (idCasillero, IPC_RMID, (struct shmid_ds *)NULL);
        // free(a->actual); // Se libera el casillero
    }
    a->actual = NULL;
    a->head = NULL;
    a->length = 0;
    a->tail = NULL;
    return;
}

// agrega un nuevo casillero al tablero
void append(struct tablero *a, int players[4], char flecha[4], char eventos[6], int signos){
	
	int idCasillero;
    struct casillero *aux;

    // guarda una dirección de memoria con tamaño suficiente para el struct tablero
    idCasillero = shmget(IPC_PRIVATE,sizeof(struct casillero),IPC_CREAT | 0666); 

    // Asigna la dirección de memoria antes guardada para el struct tablero
    aux = (struct casillero*) shmat(idCasillero, 0, 0);

    aux->next=NULL;
    updatePlayer(aux, players);
    strcpy(aux->eventos, eventos); // actualiza los eventos del casillero
    strcpy(aux->flecha, flecha); // actualiza las flechas del casillero
    aux->signos = signos;

    if (a->length == 0){
        a->head = aux;
        a->tail = aux;
        a->actual = aux;
    }
    else{
        a->tail->next = aux;
        aux->prev = a->tail;
        a->tail = aux;
    }
    a->length++;
    aux->numero = a->length; // actualiza numero de casillero
}

// actualiza el arreglo players del casillero
void updatePlayer(struct casillero *a, int jugadores[4]){
	for (int i = 0; i < 4; i++)
		a->players[i] = jugadores[i];
}

// imprime el tablero en la consola
void printTablero(struct tablero *a){
	int start, step;
	int columns;

	a->actual = a->head;

	// Imprime los últimos 9 casilleros
	start = 20; // empieza en el casillero número 20
	step = 1; // están ordenados
	columns = 9; // tiene 9 columnas
	printRow(a, start, columns, step);

	// Imprime 1 casillero
	start = 19;
	step = 1;
	columns = 1;
	printRow(a, start, columns, step);

	// Imprime los 9 casilleros intermedios
	start = 18;
	step = -1; // están en reversa
	columns = 9;
	printRow(a, start, columns, step);

	// Imprime 1 casillero
	start = 9;
	step = 1;
	columns = 1;
	printRow(a, start, columns, step);

	// Imprime los 9 primeros casilleros.
	start = 0;
	step = 1;
	columns = 9;
	printRow(a, start, columns, step);
}

// imprime todos los casilleros de una fila dado un rango númerico.
void printRow(struct tablero *a, int start, int columns, int step){
	int i, j, fill;
	// Encuentra el casillero de la fila donde debe empezar (start)
	a->actual = a->head;
	for (i = 0; i < start; i++)
		a->actual = a->actual->next;

	// respaldo para no perder el puntero
	struct casillero *aux = a->actual;

    // si start == 19, tiene que rellenar con 8 casilleros vacíos a la izquierda del real
    if (start == 9)
    	fill = 8;
    else
    	fill = 0;

    // Borde superior del casillero
    for(i = 0; i < fill; i++)
    	printf("         ");
    for(i = 0; i < columns; i++)
    	printf("┌───────┐");
    printf("\n");

    // Espacio con el número del casillero
    for(i = 0; i < fill; i++)
    	printf("         ");
    for(i = 0; i < columns; i++){
    	printf("│%s 0", a->actual->flecha);

    	if(a->actual->numero < 10)
    		printf("0");
    	
    	printf("%d %s│", a->actual->numero, a->actual->flecha);

    	if(step == 1)
    		a->actual = a->actual->next;
    	else
    		a->actual = a->actual->prev;
    }
    printf("\n");
    a->actual = aux;


    // Espacio de eventos
    for(i = 0; i < fill; i++)
    	printf("         ");
    for(i = 0; i < columns; i++){
    	printf("│ %s │", a->actual->eventos);
    	if(step == 1)
    		a->actual = a->actual->next;
    	else
    		a->actual = a->actual->prev;
    }
    printf("\n");
    a->actual = aux;

    // Espacio de jugadores
    for(i = 0; i < fill; i++)
    	printf("         ");
    for(i = 0; i < columns; i++){
    	printf("│%s", a->actual->flecha);
    	// imprime jugador 1 y 2 a la izquierda
		for (j = 0; j < 2; j++)
			if (a->actual->players[j] != 0)
				printf("%d", j + 1);
			else
				printf(" ");
		// separación meramente estética
		printf(" ");
		// jugador 3 y 4 a la derecha
		for (j = 2; j < 4; j++)
			if (a->actual->players[j] != 0)
				printf("%d", j + 1);
			else
				printf(" ");
    	printf("%s│", a->actual->flecha);
    	if(step == 1)
    		a->actual = a->actual->next;
    	else
    		a->actual = a->actual->prev;
    }
    printf("\n");

    // Borde inferior del casillero
    for(i = 0; i < fill; i++)
    	printf("         ");
    for(i = 0; i < columns; i++)
    	printf("└───────┘");
    printf("\n");
    a->actual = a->head;
}

// actualiza la posición de un jugador en el tablero (borra la antigua, agrega la nueva)
int actualizarPosicion(struct tablero *a, int player, int actualPos, int dados){
	int i, newPos;
	if (a->reverse == 0) // flujo normal
		newPos = actualPos + dados;
	else // flujo inverso
		newPos = actualPos - dados;

	if (newPos > a->length - 1)
		newPos = a->length - 1;

	if (newPos < 0)
		newPos = 0; 

	a->actual = a->head;
	for (i = 0; i < actualPos; i++)
		a->actual = a->actual->next;
	a->actual->players[player - 1] = 0; // borra al jugador de la posición antigua
	
	a->actual = a->head;
	for (i = 0; i < newPos; i++)
		a->actual = a->actual->next;
	a->actual->players[player - 1] = 1; // agrega al jugador a la posición nueva
	a->actual = a->head;

	return (newPos);
}

// genera los efectos aleatorios correspondientes a los casilleros con ? y con ??
void getEfectos(struct tablero *a, int posicion, int *efectos){
	int i;
	time_t t;
	srand((int)time(&t) % getpid()); // evita que todos los procesos tiren el mismo número "random";

	for (i = 0; i < posicion; i++)	
		a->actual = a->actual->next;

	efectos[0] = a->actual->signos; // guarda el número de signos del casillero en la primera posición de efectos

	if (efectos[0] == 1){	// 1 signo, ?
		int mov[] = {-1, 1};
		efectos[1] = rand() % 3;  // 0: block next player, 1: reverse turnos, 2: efecto de movimiento
		efectos[2] = rand() % 2;  // 0: afecta solo al jugador, 1: a los demás.  
		if (efectos[2] == 0)
			efectos[3] = mov[rand() % 2];  // cantidad de movimientos para el jugador: -1, 1
		else
			efectos[3] = -1;	// los demás siempre retroceden 1
	}

	else if (efectos[0] == 2){ // 2 signos, ??
		efectos[1] = rand() % 5; // 0: todos mueven, 1: otros mueven a casilla en blanco, 2: swap con último lugar, 3: swap con primer lugar, 4: cambio sentido tablero
		efectos[2] = -2;		// cantidad de espacios que retroceden en caso de efectos[1] = 0 
	}
	a->actual = a->head;	
}

// invierte el orden del tablero.
void reverseBoard(struct tablero *a){
	int i;
	int boardSize = a->length;
	
	if (a->reverse == 0)
		a->reverse = 1;
	else
		a->reverse = 0;

	a->actual = a->head;

	for (i = 0; i < boardSize; i++){
		// invertir número de casillero
		if (a->reverse == 1)
			a->actual->numero = boardSize - i;
		else
			a->actual->numero = i + 1;

		// invertir flechas
		if (a->reverse == 1){
			if (i == 8)
				strcpy(a->actual->flecha, "←");
			if (i == 10)
				strcpy(a->actual->flecha, "↓");
			if (i == 18)
				strcpy(a->actual->flecha, "→");
			if (i == 20)
				strcpy(a->actual->flecha, "↓");
		}
		else if (a->reverse == 0){
			if (i == 8)
				strcpy(a->actual->flecha, "↑");
			if (i == 10)
				strcpy(a->actual->flecha, "←");
			if (i == 18)
				strcpy(a->actual->flecha, "↑");
			if (i == 20)
				strcpy(a->actual->flecha, "→");
		}

		// invertir signos
		if (a->actual->signos == 1){
			a->actual->signos = 2;
			strcpy(a->actual->eventos, " ? ? ");
		}
		else if (a->actual->signos == 2){
			a->actual->signos = 1;
			strcpy(a->actual->eventos, "  ?  ");			
		}
		a->actual = a->actual->next;
	}

	a->actual = a->head;
	
	// intercambia head por tail
	if (a->reverse == 1){
		strcpy(a->head->eventos, " fin ");
		strcpy(a->head->flecha, " ");
		strcpy(a->tail->eventos, "start");
		strcpy(a->tail->flecha, "←");
	}
	else{
		strcpy(a->head->eventos, "start");
		strcpy(a->head->flecha, "→");
		strcpy(a->tail->eventos, " fin ");
		strcpy(a->tail->flecha, " ");
	}
}

// invierte el orden de los turnos
void reverseOrder(int *sentido){
	*sentido *= -1;
}

// imprime los turnos 
void printTurnos(int *orden, int turno, int *sentido){
	int i;

	if (*sentido == 1)
		printf("El orden de los turnos es de izquierda a derecha.\n\n");
	else if (*sentido == -1)
		printf("El orden de los turnos es de derecha a izquierda.\n\n");

	// borde superior de los turnos
	for(i = 0; i < 4; i++){
		if (i == turno)
			printf("┌─┐");
		else
			printf("   ");
	}
	printf("\n");

	// número de los turnos
	for(i = 0; i < 4; i++){
		if (i == turno)
			printf("│%d│", orden[i]);
		else
			printf("-%d-", orden[i]);
	}
	printf("\n");

	// borde inferior de los turnos
	for(i = 0; i < 4; i++){
		if (i == turno)
			printf("└─┘");
		else
			printf("   ");
	}
	printf("\n\n");
}

// avanza al siguiente turno
void avanzarTurno(int *turno, int *sentido){
	*turno += *sentido;
    if (*turno == 4)
        *turno = 0;
    if (*turno == -1)
        *turno = 3;
}

// consigue el número que guarda el casillero en cierta posicion
int getNumero(struct tablero *a, int posicion){
	int i, numero;
	a->actual = a->head;
	for(i = 0; i < posicion; i++)
		a->actual = a->actual->next;
	numero = a->actual->numero;
	a->actual = a->head;
	return (numero);
}

// consigue la posición de un jugador en el tablero
int getPosicion(struct tablero *a, int player){
	int i;
	a->actual = a->head;
	for (i = 0; i < a->length; i++){
		if (a->actual->players[player - 1] == 1){
			a->actual = a->head;
			return (i);
		}
		a->actual = a->actual->next;
	}
	return (-1);
}

// consigue el número del jugador que va en primer lugar si lugar = 1 o el del jugador que va en último si lugar = 0
int getPlayer(struct tablero *a, int lugar){
	int i, j;
	if ((a->reverse == 0 && lugar == 1) || (a->reverse == 1 && lugar == 0)){ // flujo normal y primer lugar o flujo invertido y ultimo lugar
		a->actual = a->tail;
		for (i = 0; i < a->length; i++){
			for(j = 0; j < 4; j++){
				if (a->actual->players[j] == 1){
					a->actual = a->head;
					return (j + 1);
				}
			}
			a->actual = a->actual->prev;
		}
	}
	else if ((a->reverse == 0 && lugar == 0) || (a->reverse == 1 && lugar == 1)){ // flujo normal y jugador último lugar o flujo invertido y primer lugar
		a->actual = a->head;
		for (i = 0; i < a->length; i++){
			for(j = 0; j < 4; j++){
				if (a->actual->players[j] == 1){
					a->actual = a->head;
					return (j + 1);
				}
			}
			a->actual = a->actual->next;
		}
	}
	return (-1);
}

// consigue el casillero en blanco que esté después de posicion en el tablero.
int getWhiteSpace(struct tablero *a, int posicion){
	int i;
	int pos = 0;

	a->actual = a->head;

	// encuentra el casillero actual
	for(i = 0; i < posicion; i++)
		a->actual = a->actual->next;

	if(a->reverse == 0){ // flujo normal en el tablero
		for (i = posicion; i < a->length - 1; i++){
			a->actual = a->actual->next;
			pos += 1;
			if (strcmp(a->actual->eventos, "     ") == 0){
				a->actual = a->head;
				return (posicion + pos);
			}
		}
	}
	else{	// tablero invertido
		for (i = posicion; i > 0; i--){
			a->actual = a->actual->prev;
			pos += 1;
			if (strcmp(a->actual->eventos, "     ") == 0){
				a->actual = a->head;
				return (posicion + pos);
			}
		}
	}
	a->actual = a->head;
	return posicion;
}

// activa el efecto de movimiento en los jugadores
int activarEfectoMovimiento(struct tablero *Tablero, int *efecto, int player, int posicion){
	int newPos;
    if (efecto[2] != player){ // número del player que lo activó.  
        if (efecto[1] != 0){ // mover una cantidad de espacios fija
        	newPos = actualizarPosicion(Tablero, player, posicion, efecto[1]);
        }
        else{ // mover hacia siguiente casillero en blanco.
            newPos = getWhiteSpace(Tablero, posicion);
            newPos = actualizarPosicion(Tablero, player, posicion, newPos - posicion);
        }
        return newPos;
    }
	return posicion;
}	

// bloquea el turno del siguiente jugador
void blockNextPlayer(int *efecto){
	efecto[0] = 2;
}

// desbloquea el turno del jugador bloqueado, después de ser bloqueado
void unblockPlayer(int *efecto, int human){
	efecto[0] = 0;
    efecto[1] = -1;
    efecto[2] = -1;
    if (human == getpid() - getppid())
        printf("Tu turno ha sido bloqueado, por lo que no puedes jugar en esta ronda.\n");
    else
        printf("El turno del jugador %d ha sido bloqueado, por lo que no puede jugar en esta ronda.\n", getpid() - getppid());
    sleep(2);
}

// activa el intercambio de jugadores. si lugar = 0, cambia con último lugar, si 1, con el primero
int activateSwap(struct tablero *Tablero, int *swap, int lugar, int posicion, int human){
    swap[0] = getpid() - getppid(); // player que provoca el cambio
    swap[1] = getPlayer(Tablero, lugar); // player afectado
    swap[2] = posicion; // posición a la que se deberá mover el player afectado
    char interes[20];

    if (lugar == 0)
    	strcpy(interes, "último");
    else
    	strcpy(interes, "primer");

    if(swap[1] != getpid() - getppid()){
        posicion = actualizarPosicion(Tablero, getpid() - getppid(), posicion, getPosicion(Tablero, swap[1]) - posicion);
        printTablero(Tablero);
        if (human == getpid() - getppid())
            printf("Te has movido a la casilla número %d correspondiente a la %s del último lugar.\n", getNumero(Tablero, posicion), interes);
        else
            printf("El jugador %d se ha movido a la casilla número %d correspondiente a la posición del %s lugar.\n", swap[0], getNumero(Tablero, posicion), interes); 
    }
    else{
        if (human == getpid() - getppid())
            printf("Ya estás en %s lugar, por lo que el intercambio no tuvo efecto.\n", interes);
        else
            printf("El jugador %d ya está en %s lugar, por lo que el intercambio no tuvo efecto.\n", swap[0], interes); 
        swap[0] = 0; // player que provoca el cambio
        swap[1] = 0; // player afectado
        swap[2] = 0; // posición a la que se deberá mover el player afectado
        
    }
    return posicion;
}

// desactiva el intercambio de jugadores (se activa después de aplicar el intercambio)
void deactivateSwap(int *swap){
	swap[0] = 0;
    swap[1] = 0;
    swap[2] = 0;
}

// retorna 0 si no hay ganador, de lo contrario retorna el número del ganador.
int checkGanador(struct tablero *a){
	int i;
	if (a->reverse == 0) // flujo normal
		a->actual = a->tail;

	else // flujo inverso
		a->actual = a->head;
	for (i = 0; i < 4; i++){
		if (a->actual->players[i] == 1){
			a->actual = a->head;
			return (i + 1);
		}
	}
	a->actual = a->head;
	return 0;
}