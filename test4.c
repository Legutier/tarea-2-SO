

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>


void* create_shared_memory(size_t size) {
  // Our memory buffer will be readable and writable:
  int protection = PROT_READ | PROT_WRITE;

  // The buffer will be shared (meaning other processes can access it), but
  // anonymous (meaning third-party processes cannot obtain an address for it),
  // so only this process and its children will be able to use it:
  int visibility = MAP_SHARED | MAP_ANONYMOUS;

  // The remaining parameters to `mmap()` are not important for this use case,
  // but the manpage for `mmap` explains their purpose.
  return mmap(NULL, size, protection, visibility, -1, 0);
}

int main(){
    pid_t pid;

    int turno;
    int roll; // almacenar valor númerico (int) de los dados
    int i;    
    int pos = 0;

    char id[5]; // almacenar el n° correspondiente al turno de cada jugador
    char fin[] = "5";

    // variables en memoria compartida, solo aprendí a compartir strings, so... F, igual es re facil hacer las conversiones.
    char player[5];
    char validation[5];
    char dados[5];
    char position[5];
    // -------------------------------
    void* shmem = create_shared_memory(5);  // Transmite los turnos (player) y las validaciones (validation) (respuesta padre-hijo)
    void* shmem2 = create_shared_memory(5); // Transmite los números de los dados
    void* shmem3 = create_shared_memory(5); // Transmite la posición (position) alcanzada por los jugadores

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
        while (pos < 30){
            // Inicialización del turno, los jugadores (hijos) esperaran hasta que 
            strcpy(dados, "0");
            memcpy(shmem2, dados, sizeof(dados)); // Se inicializa en 0. El hijo recibe respuesta cuando toma cualquier valor entre 1 y 6.

            printf("Turno del jugador número %d\n", turno);

            // Comunicaciones entre padre e hijo usando memoria compartida para envíar y recibir información de la jugada.
            sprintf(player, "%d", turno);  
            memcpy(shmem, player, sizeof(player)); // copia en memoria compartida 1 el número del jugador que tiene que jugar.

            while(strcmp(shmem, "0") != 0){};   // shmem es 0 cuando el el jugador correspondiente le responde al padre.
            sleep(1);
            printf("El jugador %s ha respondido, hora de lanzar los dados!\n\n", player);
            
            roll = (rand() % 6) + 1;
            printf("Ha salido el número %d.\n\n", roll);
            sleep(1);
            
            sprintf(dados, "%d", roll);  
            memcpy(shmem2, dados, sizeof(dados)); // copia en memoria compartida 2 el número arrojado por los dados.
            
            while(strcmp(shmem, "-1") != 0){};   // shmem es -1 cuando el el jugador correspondiente termina su jugada.

            strcpy(position, shmem3);
            sscanf(position, "%d", &pos);

            printf("El jugador %s ha terminado su turno.\n\n", player);
            sleep(2);
            turno += 1;
            if (turno > 4)
                turno = 1;
        }
        printf("El jugador %s ha ganado, felicidades!.\n\n", player);  
        memcpy(shmem, fin, sizeof(fin)); // notifica a los hijos mostrándoles un "5" que terminó la partida.
        sleep(2);  
        for(i = 0; i < 5; i++){}; // espera a que los hijos terminen sus procesos.
    }
    else {
        sprintf(id, "%d", getpid() - getppid());  
        while(1){
            sleep(1);
            strcpy(player, shmem);
            if (strcmp(player, id) == 0){
                printf("Soy el jugador número %s y es mi turno.\n", id);
                strcpy(validation, "0"); // si le toca al jugador, notifica al padre enviándole un 0.
                memcpy(shmem, &validation, sizeof(validation));
                while(strcmp(shmem2, "0") == 0){}; // Espera hasta que el padre le envíe el valor de los dados
                strcpy(dados, shmem2);
                strcpy(validation, "-1");
                sscanf(dados, "%d", &roll);
                pos+=roll;
                printf("Soy el jugador número %s y quedé en la posición %d.\n", id, pos);
                
                // Envía posición y validación al padre para terminar el turno
                sprintf(position, "%d", pos);  
                memcpy(shmem3, &position, sizeof(position));
                memcpy(shmem, &validation, sizeof(validation));
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

