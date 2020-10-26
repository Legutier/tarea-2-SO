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

#include "tablero.c"

void* create_shared_memory(size_t size) {
  // Our memory buffer will be readable and writable:
  int protection = PROT_READ | PROT_WRITE;

  // The buffer will be shared (meaning other processes can access it), but
  // anonymous (meaning third-party processes cannot obtain an address for it),
  // so only this process and its children will be able to use it:
  int visibility = MAP_SHARED | MAP_ANONYMOUS;
  return mmap(NULL, size, protection, visibility, -1, 0);
}

int main(){
    // variables para los procesos
    pid_t pid, wpid;
    time_t t;   // variable time_t para evitar que los random usen la misma seed constantemente

    int roll; // almacenar valor númerico de los dados
    int i;    
    int posicion = 0; // posición de los jugadores
    int numeroTablero; // número de una casilla del tablero.
    int status = 0; // usada para esperar que los hijos terminen sus procesos.
    int human; // número del jugador no controlado por la cpu.
    int activate = 1; // 0 o 1, indica si activa o no el efecto de la cuadrícula, 1 por default para la cpu.
    int efectos[4]; // almacena los efectos de las cuadriculas especiales (? y ??)
    int orden[4]; // orden de los turnos


    char id[5]; // almacenar el n° correspondiente al turno de cada jugador
    char lock[] = "0";
    char fin[] = "5";
    char instruccion[20]; // guarda el string "avanzar" o "retroceder" según el tipo de efecto activado (estético)

    // variables usadas para memoria compartida
    char player[5];
    char validation[5];

    int idTurno;
    int *turno;

    int idSentido;
    int *sentido; // 1 si los turnos van de izquierda a derecha, -1 si van de derecha a izquierda.

    int idEfecto;
    int *efecto;

    int idSwap;
    int *swap;


    int idTablero;
    struct tablero *Tablero;
    // -------------------------------
    // Crea espacios de memoria compartida para variables de jugadores
    void* shmem = create_shared_memory(5);  // Transmite los turnos (player) 
    void* shmem2 = create_shared_memory(5); // Transmite la validación (validation) de los jugadores
    
    
    idTablero = shmget(IPC_PRIVATE,sizeof(struct tablero),IPC_CREAT | 0666); // guarda una dirección de memoria con tamaño suficiente para el struct tablero
    Tablero = (struct tablero*) shmat(idTablero, 0, 0); // Asigna la dirección de memoria antes guardada para el struct tablero

    idTurno = shmget(IPC_PRIVATE,sizeof(int),IPC_CREAT | 0666);
    turno = (int *) shmat(idTurno, 0, 0); 

    idSentido = shmget(IPC_PRIVATE,sizeof(int),IPC_CREAT | 0666); 
    sentido = (int *) shmat(idSentido, 0, 0); 

    idEfecto = shmget(IPC_PRIVATE,sizeof(int)*3,IPC_CREAT | 0666); 
    efecto = (int *) shmat(idEfecto, 0, 0); 

    idSwap = shmget(IPC_PRIVATE,sizeof(int)*3,IPC_CREAT | 0666); 
    swap = (int *) shmat(idSwap, 0, 0); 

    efecto[0] = 0;  // 0: no efecto, 1: los demás se mueven, 2: todos se mueven, 3: swap con ultimo lugar, 4: swap con primer lugar
    efecto[1] = 0;  // Cantidad de espacios a mover, 0: hasta casilla en blanco. Solo se considera cuando efecto[0] es 1 o 2.
    efecto[2] = -1; // número del player que lo activo.

    swap[0] = 0; // n° del jugador que provocó el swap
    swap[1] = 0; // n° del jugador afectado
    swap[2] = 0; // nueva posicion para jugador afectado.

    init(Tablero); // Inicializa el tablero con los jugadores en la posición de partida.
    printTablero(Tablero);

    // inicialización de los turnos
    strcpy(player, "0"); 
    memcpy(shmem, player, sizeof(player)); // Se inicializa en 0. Los hijos reciben respuesta cuando es de 1 a 5.
    printf("Hay 4 jugadores enumerados desde el 1 al 4.\n\n");
    
    for(i = 0; i < 4; i++){
        printf("Ingrese el número del jugador que tendrá el turno número %d: ", i+1);
        scanf("%d", turno);
        orden[i] = *turno;
        printf("\n\n");
    }

    printf("Si ingresa un número distinto al número que tienen los jugadores, el juego iniciará con todos los jugadores controlados por la cpu.\n");
    printf("Ingrese el número del jugador que desea controlar: ");
    scanf("%d", &human);
    printf("\n\n");
    if(human == 5)
        printf("---------Debug mode enabled, controlará las acciones de todos los jugadores.---------\n\n\n");


    // Forks
    for(i = 0; i < 4; i++){
        pid = fork();
        if(pid == 0)
            break;   
    }

    if (pid > 0){
        *turno = 0;
        *sentido = 1;   // turnos de izquierda a derecha
        int ganador = 0; // int con el número del jugador ganador.
        // Padre maneja los turnos hasta que un jugador haya ganado (llegado a la posición 30)
        while (ganador == 0){
            // solo imprime los mensajes si es que los jugadores no están bajo algún efecto de movimiento.
            if(efecto[0] != 1 && swap[0] == 0){ 
                printTurnos(orden, *turno, sentido);
                printf("Turno del jugador número: %d.\n", orden[*turno]);
                sleep(1);    
            }
            
            // Comunicaciones entre padre e hijo usando memoria compartida para envíar y recibir información de la jugada.
            sprintf(player, "%d", orden[*turno]);  
            memcpy(shmem, player, sizeof(player));  // copia en memoria compartida 1 el número del jugador que tiene que jugar.

            while(strcmp(shmem2, "1") != 0){};      // shmem2 es "1" cuando el el jugador correspondiente termina su jugada.

            memcpy(shmem2, &lock, sizeof(lock));     // restablece el valor de la verificación a "0" para el siguiente jugador.

            if(efecto[0] == 0 && swap[0] == 0)
                printf("El jugador %s ha terminado su turno.\n\n", player);

            // printf("DEBUG: player en primer lugar: %d, player en último lugar: %d\n\n", getPlayer(Tablero, 1), getPlayer(Tablero, 0));
    
            printf("----------------------------------------\n\n");
            sleep(2);

            
            avanzarTurno(turno, sentido);

            ganador = checkGanador(Tablero);
        }

        printf("El jugador %d ha ganado, felicidades!.\n\n", ganador);  
        memcpy(shmem, fin, sizeof(fin)); // notifica a los hijos mostrándoles un "5" que terminó la partida.
        sleep(2);  
        while ((wpid = wait(&status)) > 0); // espera a que los hijos terminen sus procesos.

        // Borra el tablero.    
        clear(Tablero);
        // Se libera la memoria usada para memoria compartida
        shmdt ((char *)Tablero);
        shmctl (idTablero, IPC_RMID, (struct shmid_ds *)NULL);

        shmdt ((char *)turno);
        shmctl (idTurno, IPC_RMID, (struct shmid_ds *)NULL);

        shmdt ((char *)sentido);
        shmctl (idSentido, IPC_RMID, (struct shmid_ds *)NULL);

        shmdt ((char *)efecto);
        shmctl (idEfecto, IPC_RMID, (struct shmid_ds *)NULL);

        shmdt ((char *)swap);
        shmctl (idSwap, IPC_RMID, (struct shmid_ds *)NULL);
    }
    else { // Hijos juegan
        // ---------------------------------
        // DEBUG
        if(human == 5) 
            human = getpid() - getppid(); // permite controlar a los 4 jugadores
        //--------------------------------
        // Si activate es 0, le preguntará al jugador si quiere activar los efectos. La cpu toma el valor 1 por default (no pregunta)
        if (human == getpid() - getppid()) 
            activate = 0;
        else
            activate = 1;

        sprintf(id, "%d", getpid() - getppid());  
        while(1){
            strcpy(player, shmem);  // shmem guarda el n° del jugador que debe jugar (turno)
            if (strcmp(player, id) == 0){

                // Cambia el turno guardado en memoria compartida para que sea "0" 
                // evita que el jugador vuelva a entrar al terminar de iterar.
                memcpy(shmem, &lock, sizeof(lock));
                
                // revisión de efectos
                // si el efecto lo consiguió el jugador actual se da por completada la ronda y el próximo jugador tendrá turno normal
                if(efecto[2] == getpid() - getppid()){
                    efecto[0] = 0;
                    efecto[1] = -1;
                    efecto[2] = -1;
                }
                // si hay un efecto en juego, se realiza éste en vez de lanzar los dados.
                else if (efecto[0] != 0){ 
                    if (efecto[0] == 1){ // los demás jugadores se mueven x espacios
                        posicion = activarEfectoMovimiento(Tablero, efecto, getpid() - getppid(), posicion);
                        numeroTablero = getNumero(Tablero, posicion);
                        printTablero(Tablero);
                        if (human == getpid() - getppid())
                            printf("Quedaste en la casilla número %d del tablero debido al efecto activo.\n", numeroTablero);
                        else
                            printf("El jugador %s ha quedado en la casilla número %d del tablero debido al efecto activo.\n", id, numeroTablero);
                        sleep(2);
                    }
                    else if(efecto[0] == 2){ // el jugador es bloqueado y pierde su turno
                        unblockPlayer(efecto, human);
                    }
                }
                // revisa efecto de swap entre dos jugadores.
                else if(swap[0] != 0){
                    // si hay swap solo juega el afectado y el que lo provocó
                    if(swap[0] == getpid() - getppid()){ // jugador que provocó swap
                        deactivateSwap(swap);
                    }
                    else if(swap[1] == getpid() - getppid()){ // jugador afectado por swap
                        roll = adjustMovement(Tablero, swap[2] - posicion);
                        posicion = actualizarPosicion(Tablero, getpid() - getppid(), posicion, roll); 
                        numeroTablero = getNumero(Tablero, posicion);
                        printTablero(Tablero);
                        if (human == getpid() - getppid())
                            printf("Intercambiaste posiciones con el jugador %d, quedando en la casilla número %d del tablero.\n", swap[0], numeroTablero);
                        else
                            printf("El jugador %s ha quedado en la casilla número %d del tablero luego de intercambiar posiciones con el jugador número %d.\n", id, numeroTablero, swap[0]);
                        sleep(2);
                    }
                }
                
                else{ // lanza el dado
                    srand((int)time(&t) % getpid()); // evita que todos los procesos tiren el mismo número "random";
                    roll = (rand() % 6) + 1;
                    if (human == getpid() - getppid())
                        printf("Obtuviste el número %d en los dados.\n", roll);
                    else
                        printf("El jugador %s ha obtenido el número %d en los dados.\n", id, roll);
                    sleep(2);

                    // actualiza su posición
                    posicion = actualizarPosicion(Tablero, getpid() - getppid(), posicion, roll);
                    numeroTablero = getNumero(Tablero, posicion);
                    printTablero(Tablero);

                    if (human == getpid() - getppid())
                        printf("\nQuedaste en la casilla número %d del tablero.\n", numeroTablero);
                    else
                        printf("\nEl jugador %s ha quedado en la casilla número %d del tablero.\n", id, numeroTablero);
                    sleep(1);
                    // jugador obtiene los efectos de la casilla 
                    getEfectos(Tablero, posicion, efectos);
                    
                    // mensajes relacionados a los efectos de la casilla
                    if (efectos[0] == 0) // sin efectos
                        printf("La casilla número %d no tiene efectos.\n", numeroTablero);

                    else if(efectos[0] == 1)
                        printf("La casilla número %d tiene efectos del tipo ?.\n\n", numeroTablero);

                    else if(efectos[0] == 2)
                        printf("La casilla número %d tiene efectos del tipo ??.\n\n", numeroTablero);
                    sleep(2);

                    if (activate == 0 && efectos[0] != 0){
                        if (efectos[0] == 1)
                            printf("¿Quieres activar el efecto ? de ésta casilla?\n");
                        else if(efectos[0] == 2)
                            printf("¿Quieres activar el efecto ?? de ésta casilla?\n");

                        printf("Ingresa 1 para activarlo, 0 para ignorarlo: ");
    
                        scanf("%d", &activate);
                        printf("\n");
                        if(activate == 2){  
                            debugMode(efectos);
                            activate = 1;
                        }
                    }

                    if(efectos[0] == 1){  // efectos de 1 signo ?
                        if (activate == 1){
                            if (efectos[1] == 0){ // block next player
                                printf("Efecto ? activado: el siguiente jugador pierde su turno.\n");
                                blockNextPlayer(efecto);
                            }
                            else if(efectos[1] == 1){ // reverse turnos
                                printf("Efecto ? activado: el orden de los turnos ha sido invertido.\n");
                                reverseOrder(sentido);
                            }
                            else if(efectos[1] == 2){ // efecto de movimiento
                                if (efectos[2] == 0){ // afecta solo al jugador
                                    if(efectos[3] > 0)
                                        strcpy(instruccion, "avanzar");
                                    else
                                        strcpy(instruccion, "retroceder");

                                    if (human == getpid() - getppid())
                                        printf("Efecto ? activado: debes %s %d espacio.\n", instruccion, efectos[3]);
                                    else
                                        printf("Efecto ? activado: el jugador %s debe %s %d espacio.\n", id, instruccion, efectos[3]);
                                    sleep(3);
                                    posicion = actualizarPosicion(Tablero, getpid() - getppid(), posicion, efectos[3]); // efectos[3] guarda la cantidad de espacios a mover (-1, 1)
                                    numeroTablero = getNumero(Tablero, posicion);

                                    printTablero(Tablero);
                                        if (human == getpid() - getppid())
                                        printf("Quedaste en la casilla %d tras %s %d espacio debido al efecto activado.\n", numeroTablero, instruccion, efectos[3]);
                                    else
                                        printf("El jugador %s quedó en la casilla %d tras %s %d espacio debido al efecto activado.\n", id, numeroTablero, instruccion, efectos[3]);     
                                }
                                else if(efectos[2] == 1){ // afecta a los demás
                                    printf("Efecto ? activado: los demás jugadores deberán retroceder 1 espacio.\n");
                                    efecto[0] = 1;  // 0: no efecto, 1: los demás se mueven, 2: siguiente jugador es bloqueado
                                    efecto[1] = efectos[3];  // Cantidad de espacios a mover, 0: hasta casilla en blanco. Solo se considera cuando efecto[0] es 1.
                                    efecto[2] = getpid() - getppid(); // n° del player que activó el efecto.
                                }
                                sleep(3);
                            }
                        }
                        else
                            printf("Has ignorado el efecto ? de ésta casilla.\n");
                        sleep(1);    
                    }
                    else if(efectos[0] == 2){ // efectos de dos signos ??
                        if (activate == 1){
                            if (efectos[1] == 0){ // todos los jugadores se mueven
                                if(efectos[3] > 0)
                                    strcpy(instruccion, "avanzar");
                                else
                                    strcpy(instruccion, "retroceder");

                                printf("Efecto ?? activado: todos los jugadores deben %s %d espacios.\n", instruccion, efectos[2]);
                                posicion = actualizarPosicion(Tablero, getpid() - getppid(), posicion, efectos[2]);
                                numeroTablero = getNumero(Tablero, posicion);
                                
                                printTablero(Tablero);
                                if (human == getpid() - getppid())
                                    printf("Quedaste en la casilla número %d del tablero debido al efecto activo.\n", numeroTablero);
                                else
                                    printf("El jugador %s ha quedado en la casilla número %d del tablero debido al efecto activo.\n", id, numeroTablero);
                                sleep(2);
                                efecto[0] = 1;  // 0: no efecto, 1: los demás se mueven, 2: siguiente jugador es bloqueado
                                efecto[1] = efectos[2];  // Cantidad de espacios a mover, 0: hasta casilla en blanco. Solo se considera cuando efecto[0] es 1.
                                efecto[2] = getpid() - getppid();
                            }
                            else if (efectos[1] == 1){ // los demás jugadores avanzan a siguiente casilla en blanco
                                printf("Efecto ?? activado: todos los demás jugadores deben avanzar a la siguiente casilla en blanco.\n");
                                efecto[0] = 1;  // 0: no efecto, 1: los demás se mueven, 2: siguiente jugador es bloqueado
                                efecto[1] = 0;  // Cantidad de espacios a mover, 0: hasta casilla en blanco. Solo se considera cuando efecto[0] es 1.
                                efecto[2] = getpid() - getppid();
                            }
                            else if (efectos[1] == 2){ // swap con último lugar                                
                                if (human == getpid() - getppid())
                                    printf("Tienes que cambiar posiciones con el jugador en el último lugar.\n\n");
                                else
                                    printf("El jugador %s tiene que cambiar posiciones con el jugador en el último lugar.\n\n", id); 
                                sleep(2);
                                posicion = activateSwap(Tablero, swap, 0, posicion, human);
                            }
                            else if (efectos[1] == 3){ // swap con primer lugar                                
                                if (human == getpid() - getppid())
                                    printf("Tienes que cambiar posiciones con el jugador en el primer lugar.\n\n");
                                else
                                    printf("El jugador %s tiene que cambiar posiciones con el jugador en el primer lugar.\n\n", id); 

                                sleep(2);
                                posicion = activateSwap(Tablero, swap, 1, posicion, human);    
                            }
                            else if (efectos[1] == 4){ // invertir sentido del tablero                                
                                reverseBoard(Tablero);
                                printf("El tablero ha sido invertido debido al efecto de la casilla.\n\n");
                                sleep(2);
                                printf("----------------------------------------\n\n");
                                printTablero(Tablero);
                            }
                        }
                        else
                            printf("Has ignorado el efecto ?? de ésta casilla.\n");
                        sleep(2);        
                    }

                    if (human == getpid() - getppid()) // 
                        activate = 0;
                    else
                        activate = 1;
                }
                // Guarda el string "1" en memoria compartida, indicando que terminó su turno
                strcpy(validation, "1");
                memcpy(shmem2, &validation, sizeof(validation));
            }
            // Cuando termine el juego el padre colocara el turno "5" en memoria para que los hijos terminen
            if(strcmp(player, fin) == 0){
                break;
            }
        }
    }

    if(pid > 0)
        printf("Fin del programa.\n");
    else
        printf("Soy el jugador %d y me fui a mimir.\n", getpid() - getppid());

    return 0;
}           