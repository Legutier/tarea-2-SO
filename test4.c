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

    int turno;
    int roll; // almacenar valor númerico de los dados
    int i;    
    int oldPos = 0;
    int newPos = 0;
    int status = 0; // usada para esperar que los hijos terminen sus procesos.
    int sizeBoard = 28;


    char id[5]; // almacenar el n° correspondiente al turno de cada jugador
    char lock[] = "0";
    char fin[] = "5";

    // variables usadas para memoria compartida
    char player[5];
    char validation[5];
    char position[5];  

    int idTablero;
    struct tablero *Tablero;
    // -------------------------------
    // Crea espacios de memoria compartida para variables de jugadores
    void* shmem = create_shared_memory(5);  // Transmite los turnos (player) 
    void* shmem2 = create_shared_memory(5); // Transmite la validación de los jugadores
    void* shmem3 = create_shared_memory(5); // Transmite la posición (position) alcanzada por los jugadores
    
    
    // guarda una dirección de memoria con tamaño suficiente para el struct tablero
    // http://www.csl.mtu.edu/cs4411.ck/www/NOTES/process/shm/shmget.html 
    idTablero = shmget(IPC_PRIVATE,sizeof(struct tablero),IPC_CREAT | 0666); 

    // Asigna la dirección de memoria antes guardada para el struct tablero
    Tablero = (struct tablero*) shmat(idTablero, 0, 0);

    // Inicializa el tablero con los jugadores en la posición de partida.
    init(Tablero);
    printTablero(Tablero);

    // inicialización de los turnos
    strcpy(player, "-1"); 
    memcpy(shmem, player, sizeof(player)); // Se inicializa en -1. El padre recibe respuesta cuando es 0 y los hijos cuando es de 1 a 4.
    
    // Forks
    for(i = 0; i < 4; i++){
        pid = fork();
        if(pid == 0)
            break;   
    }

    if (pid > 0){
        turno = 1;
        // Padre maneja los turnos hasta que un jugador haya ganado (llegado a la posición 30)
        while (newPos < sizeBoard){
            // Inicialización del turno, los jugadores (hijos) esperaran hasta que 
            printf("Turno del jugador número: %d.\n", turno);

            // Comunicaciones entre padre e hijo usando memoria compartida para envíar y recibir información de la jugada.
            sprintf(player, "%d", turno);  
            memcpy(shmem, player, sizeof(player));  // copia en memoria compartida 1 el número del jugador que tiene que jugar.

            while(strcmp(shmem2, "1") != 0){};      // shmem2 es "1" cuando el el jugador correspondiente termina su jugada.

            memcpy(shmem2, &lock, sizeof(lock));     // restablece el valor de la verificación a "0" para el siguiente jugador.

            strcpy(position, shmem3);               // antes de terminar la jugada, el jugador guarda su posición en la memoria compartida 3.
            sscanf(position, "%d", &newPos);

            printf("El jugador %s ha terminado su turno.\n\n", player);
            printf("----------------------------------------\n\n");
            sleep(2);
            turno += 1;
            if (turno > 4)
                turno = 1;
        }
        printf("El jugador %s ha ganado, felicidades!.\n\n", player);  
        memcpy(shmem, fin, sizeof(fin)); // notifica a los hijos mostrándoles un "5" que terminó la partida.
        sleep(2);  
        while ((wpid = wait(&status)) > 0); // espera a que los hijos terminen sus procesos.

        // Borra el tablero.    
        clear(Tablero);
        // Se libera la memoria del Tablero
        shmdt ((char *)Tablero);
        shmctl (idTablero, IPC_RMID, (struct shmid_ds *)NULL);
    }
    else {
        // Hijos juegan
        sprintf(id, "%d", getpid() - getppid());  
        while(1){
            sleep(1);
            strcpy(player, shmem);  // shmem guarda el n° del jugador que debe jugar (turno)
            if (strcmp(player, id) == 0){

                // Cambia el turno guardado en memoria compartida para que sea "0" 
                // evita que el jugador vuelva a entrar al terminar de iterar.
                memcpy(shmem, &lock, sizeof(lock));

                printf("Soy el jugador número %s y es mi turno.\n", id);
                srand((int)time(&t) % getpid()); // evita que todos los procesos tiren el mismo número "random";
                roll = (rand() % 6) + 1;
                printf("Valor de los dados: %d.\n", roll);
                newPos = oldPos + roll;

                // verifica que no se salga del tablero
                if(newPos > 28)
                    newPos = 28;

                sleep(1);
                // jugador actualiza su posición en el Tablero ubicado en memoria compartida.
                actualizarPosicion(Tablero, getpid() - getppid(), oldPos, newPos);

                oldPos = newPos;

                printTablero(Tablero);

                printf("\n\nSoy el jugador número %s y quedé en la posición %d del tablero.\n", id, newPos + 1);
                
                // Guarda la nueva posición en memoria compartida
                sprintf(position, "%d", newPos);  
                memcpy(shmem3, &position, sizeof(position));

                // Guarda el string "1" en memoria compartida, indicando que terminó su turno
                strcpy(validation, "1");
                memcpy(shmem2, &validation, sizeof(validation));
                sleep(1);

            }
            // Cuando termine el juego el padre colocara el turno "5" en memoria para que los hijos terminen
            if(strcmp(player, fin) == 0){
                break;
            }
        }
    }

    printf("Soy %d y me fui a mimir.\n", getpid());
    return 0;
}           

