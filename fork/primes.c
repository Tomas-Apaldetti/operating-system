#define WRITE 1
#define READ 0

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

long readLimit(char *);

void generator(int[2], long);

void sieveWork(int[2], int[2], long);

void sieve(int[2]);

long
readLimit(char *argument)
{
	errno = 0;
	char *rest;
	long result = strtol(argument, &rest, 10);
	if (errno == ERANGE || errno == EINVAL || rest[0] != '\0') {
		// Error transformando o no es numero enteramente(tiene
		// caracteres extras)
		perror("El argumento debe ser un numero representable en "
		       "64bits");
		exit(-1);
	}
	if (result <= 1) {
		perror("El argumento debe ser un numero mayor a 1");
		exit(-1);
	}

	return result;
}


// Envia por la pipe los numeros hasta el limite pedido
// cerrando la pipe una vez terminado
void
generator(int writePipe[2], long limit)
{
	close(writePipe[READ]);

	for (long i = 2; i <= limit; i++) {
		int writeResult = write(writePipe[WRITE], &i, sizeof(i));
		if (writeResult < 0) {
			close(writePipe[WRITE]);
			perror("Error escribiendo en el generador\n");
			exit(-1);
		}
	}

	close(writePipe[WRITE]);
}

// Lee de la pipe hasta que se haya cerrado el extremo de excritura,
// ya sea del generador o de un tamiz superior.
void
sieveWork(int writePipe[2], int readPipe[2], long divisor)
{
	close(writePipe[READ]);
	long next;
	while (read(readPipe[READ], &next, sizeof(next)) > 0)
		if (next % divisor != 0) {
			int writeResult =
			        write(writePipe[WRITE], &next, sizeof(next));
			if (writeResult < 0) {
				close(readPipe[READ]);
				close(writePipe[WRITE]);
				perror("Error escribiendo desde tamiz");
				fprintf(stderr, "%ld * n\n", divisor);
				exit(-1);
			}
		}

	close(readPipe[READ]);
	close(writePipe[WRITE]);
}

// Setup del tamiz, selecciona el primer numero recibido y se pone a escuchar
// para pasar cualquier numero que no el primer numero no divida exacto.
// Setea la pipe para el proximo tamiz, en caso de que sea necesario.
// En caso de no ser necesario, el mismo tamiz se encarga de terminarse
// correctamente.
void
sieve(int parentSieveReadPipe[2])
{
	close(parentSieveReadPipe[WRITE]);

	long divisor = 0;
	// Cualquier sieve de P*n crea el sieve para el siguiente primo
	// En el caso de que P+1 este fuera del rango especificado, si no se
	// verifica que la lectura se desbloqueo porque nunca se escribio nada
	// se crearan infinitos forks, con infinitas pipes
	if (read(parentSieveReadPipe[READ], &divisor, sizeof(divisor)) <= 0) {
		// Nunca se escribio nada en esta pipe, la lectura fue
		// desbloqueada
		// por haberse cerrado la escritura
		close(parentSieveReadPipe[READ]);
		return;
	}

	printf("primo %ld \n", divisor);

	int parentToChildFDs[2];
	int pcResult = pipe(parentToChildFDs);
	if (pcResult < 0) {
		close(parentSieveReadPipe[READ]);
		exit(-1);
	}

	int fResult = fork();
	if (fResult < 0) {
		close(parentToChildFDs[WRITE]);
		close(parentToChildFDs[READ]);
		close(parentSieveReadPipe[READ]);
		perror("Error en el fork");
		exit(-1);
	}

	if (fResult == 0) {
		close(parentSieveReadPipe[READ]);
		sieve(parentToChildFDs);
	} else {
		sieveWork(parentToChildFDs, parentSieveReadPipe, divisor);
		pid_t result;
		if ((result = wait(NULL)) < 0) {
			perror("Error mientras esperaba a mi hijo");
			exit(-1);
		};
	}
}

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		perror("Faltan argumentos");
		exit(-1);
	}

	long limit = readLimit(argv[1]);

	int parentToChildFDs[2];

	int pcResult = pipe(parentToChildFDs);
	if (pcResult < 0) {
		perror("Error creando pipe");
		exit(-1);
	}

	int fResult = fork();
	if (fResult < 0) {
		close(parentToChildFDs[WRITE]);
		close(parentToChildFDs[READ]);
		perror("Error en el fork");
		exit(-1);
	}

	if (fResult == 0) {
		sieve(parentToChildFDs);
	} else {
		generator(parentToChildFDs, limit);
		pid_t result;
		if ((result = wait(NULL)) < 0) {
			perror("Error mientras esperaba a mi hijo");
			exit(-1);
		};
	}

	return 0;
}