struct casillero{
  struct casillero* next;	// puntero al siguiente casillero
  struct casillero* prev;	// puntero al casillero anterior
  char flecha[4];			// char con la flecha que debe usar el casillero, "→" tiene size 4, lo mismo para las otras
  int players[4]; 			// player[i] será 1 si el player (i + 1) está en este casillero, 0 de lo contrario.
  char eventos[6]; 			// eventos, toma valores "start", "     ", "  ?  ", " ? ? " y " fin "
};

struct tablero{
  struct casillero* actual;
  struct casillero* head;
  struct casillero* tail;
  int length;
};

void init(struct tablero *a);
void clear(struct tablero *a);
void append(struct tablero *a, int players[4], char flecha[4], char eventos[6]);

void updatePlayer(struct casillero *a, int jugadores[4]);
void updateEvento(struct casillero *a, char eventos[6]);
void updateFlechas(struct casillero *a, char flecha[4]);

void printTablero(struct tablero *a);
void printRow(struct tablero *a, int start, int columns, int step);

void actualizarPosicion(struct tablero*a, int player, int oldPos, int newPos);