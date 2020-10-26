struct casillero{
  struct casillero* next;	// puntero al siguiente casillero
  struct casillero* prev;	// puntero al casillero anterior
  char flecha[4];			// char con la flecha que debe usar el casillero, "→" tiene size 4, lo mismo para las otras
  char eventos[6]; 			// eventos, toma valores "start", "     ", "  ?  ", " ? ? " y " fin "
  int numero;
  int players[4]; 			// player[i] será 1 si el player (i + 1) está en este casillero, 0 de lo contrario.
  int idMemoria;			// id de la memoria donde se guarda el casillero.
  int signos;	// cantidad de signos ? que tiene el casillero (0, 1 o 2)
};

struct tablero{
  struct casillero* actual;
  struct casillero* head;
  struct casillero* tail;
  int length;
  int reverse; // 0 si el tablero sigue flujo normal, 1 si está en reverse.
};

void init(struct tablero *a);
void clear(struct tablero *a);
void append(struct tablero *a, int players[4], char flecha[4], char eventos[6], int signos);
void updatePlayer(struct casillero *a, int jugadores[4]);
void printTablero(struct tablero *a);
void printRow(struct tablero *a, int start, int columns, int step);
int actualizarPosicion(struct tablero *a, int player, int actualPos, int dados);
void getEfectos(struct tablero *a, int posicion, int *efectos);
void reverseBoard(struct tablero *a);
void reverseOrder(int *sentido);
void printTurnos(int *orden, int turno, int *sentido);
void avanzarTurno(int *turno, int *sentido);
int getPosicion(struct tablero *a, int player);
int getPlayer(struct tablero *a, int lugar);
int getWhiteSpace(struct tablero *a, int posicion);
int activarEfectoMovimiento(struct tablero *Tablero, int *efecto, int player, int posicion);
void blockNextPlayer(int *efecto);
void unblockPlayer(int *efecto, int human);
int activateSwap(struct tablero *Tablero, int *swap, int lugar, int posicion, int human);
void deactivateSwap(int *swap);
int checkGanador(struct tablero *a);