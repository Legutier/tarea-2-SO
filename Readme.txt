Integrantes:	
	- Lukas Gutiérrez	201673059-1		
	- Ignacio Jorquera	201873561-2	

Instrucciones para correr el programa:
- Compilar:	make
- Ejecutar:	./juego
- Limpiar:	make clean

**El programa se probó en Ubuntu 20.04.1 con GCC 9.3.0**

Instrucciones para garantizar el funcionamiento del programa:
- Al crear los jugadores, estos deben tener números del 1 al 4 sin repetir (el orden define el orden inicial de los turnos).

Instrucciones para ingresar al modo debug y revisar mejor los casos de pruebas:
- Ingresar el número 5 al elegir el jugador permite controlar a los 4 jugadores.
- Ingresar el número 2 como respuesta a activar o no activar el efecto de la casilla permite elegir cualquiera de los efectos implementados más un par extra.


El programa consta de 5 procesos, 1 padre que organiza los turnos y 4 hijos que se dedican a jugar.
El tablero del juego es un struct tablero que almacena los structs de cada casillero. Tanto el tablero como cada casillero se encuentran en memoria compartida.

El padre revisa el número del jugador que debe jugar, luego guarda este número en memoria compartida. 

- Los hijos estarán constantemente leyendo ese espacio de memoria hasta que lean el número correspondiente a su turno.
- Cuando le toca a un jugador, primero revisará si hay algún efecto activado en memoria compartida de algún otro jugador (saltar turno, avanzar/retroceder espacios, etc).
    - Si hay efectos activados, el jugador interactuará con el tablero en base al efecto y luego termina su turno.
    - Si no hay efectos activados, el jugador lanza los dados, actualiza su posición en el tablero y revisa los efectos de la casilla.
	- Si la casilla afecta al jugador, el efecto se activa de inmediato.
	- Si la casilla afecta a los demás jugadores, se activa un flag en memoria compartida para que cada jugador active el efecto de manera independiente.
	- Una vez que todos los jugadores afectados activan el efecto, el flag se reinicia y los turnos se reanudan de manera normal.

- Cuando un jugador termina su turno, este le envía un mensaje al padre mediante pipes.

Al recibir el mensaje del hijo, el padre revisa si algún jugador llegó a la última casilla.
- Si un jugador llegó al final     -> ese jugador gana, padre notifica a los hijos poniendo cierto valor en memoria compartida y luego el juego termina.
- Si ningún jugador llegó al final -> padre hace avanzar los turnos en el sentido correspondiente (el sentido está en memoria compartida, es modificado por algún efecto activado por los hijos) y el juego continua.