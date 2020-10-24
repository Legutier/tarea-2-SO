#include "tablero.h"


// Construye el tablero inicial
void init(struct tablero *a){
    // componentes de los casilleros
    int players[4];
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
    		if (i == 0 && j == 0)
    			strcpy(eventos, "start");
    		else if(i == 4 && j == 8)
    			strcpy(eventos, " fin ");

    		else if((i == 0 && (j == 2 || j == 4 || j == 6)) || (i == 2 && (j == 2 || j == 4)) || (i == 4 && j % 2 == 1))
    			strcpy(eventos, "  ?  ");

    		else if((i == 2 && j == 6) || (i == 4 && (j == 2 || j == 4 || j == 6)))
    			strcpy(eventos, " ? ? ");
    		else
    			strcpy(eventos, "     ");
    		// Agrega el casillero con los valores obtenidos
    		append(a, players, flecha, eventos);
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
void append(struct tablero *a, int players[4], char flecha[4], char eventos[6]){
	
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
void actualizarPosicion(struct tablero*a, int player, int oldPos, int newPos){
	int i;
	for (i = 0; i < oldPos; i++)
		a->actual = a->actual->next;
	a->actual->players[player - 1] = 0; // borra al jugador de la posición antigua
	a->actual = a->head;

	for (i = 0; i < newPos; i++)
		a->actual = a->actual->next;
	a->actual->players[player - 1] = 1; // agrega al jugador a la posición nueva
	a->actual = a->head;
}
