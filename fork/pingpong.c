#define WRITE 1
#define READ 0

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>

void childTask(int[2], int[2], int);

void parentTask(int[2], int[2], int);

int
main(void)
{
	int parentToChildFDs[2];
	int childToParentFDs[2];

	int pcResult = pipe(parentToChildFDs);
	if (pcResult < 0) {
		perror("Error creando pipe");
		exit(-1);
	}

	int cpResult = pipe(childToParentFDs);
	if (cpResult < 0) {
		close(parentToChildFDs[WRITE]);
		close(parentToChildFDs[READ]);
		perror("Error creando pipe");
		exit(-1);
	}

	srandom(time(NULL));
	printf("Hola, soy PID <%d>:\n", getpid());
	printf("  - primer pipe me devuelve: [%d, %d]\n",
	       parentToChildFDs[READ],
	       parentToChildFDs[WRITE]);
	printf("  - segundo pipe me devuelve: [%d, %d]\n",
	       childToParentFDs[READ],
	       childToParentFDs[WRITE]);

	int fResult = fork();
	if (fResult < 0) {
		close(parentToChildFDs[WRITE]);
		close(parentToChildFDs[READ]);
		close(childToParentFDs[WRITE]);
		close(childToParentFDs[READ]);
		perror("Error en el fork");
		exit(-1);
	}

	if (fResult == 0) {
		childTask(parentToChildFDs, childToParentFDs, fResult);
	} else {
		parentTask(childToParentFDs, parentToChildFDs, fResult);
	}

	return 0;
}

void
childTask(int listenPipe[2], int writePipe[2], int forkResult)
{
	close(listenPipe[WRITE]);
	close(writePipe[READ]);
	long int received = 0;
	int readResult = read(listenPipe[READ], &received, sizeof(received));
	close(listenPipe[READ]);

	if (readResult < 0) {
		close(writePipe[WRITE]);
		perror("Error en la lectura desde el hijo\n");
		exit(-1);
	}

	printf("Donde fork me devuelve <%d>:\n", forkResult);
	printf("  - getpid me devuelve: <%d>\n", getpid());
	printf("  - getppid me devuelve: <%d>\n", getppid());
	printf("  - recibo valor <%ld> vía fd=%d\n", received, listenPipe[READ]);
	printf("  - reenvío valor en fd=%d y termino\n", writePipe[WRITE]);

	int writeResult = write(writePipe[WRITE], &received, sizeof(received));
	close(writePipe[WRITE]);

	if (writeResult < 0) {
		perror("Error en la escritura desde el hijo\n");
		exit(-1);
	}
}

void
parentTask(int listenPipe[2], int writePipe[2], int forkResult)
{
	close(listenPipe[WRITE]);
	close(writePipe[READ]);

	int pid = getpid();

	long int sent = random();

	printf("Donde fork me devuelve <%d>:\n", forkResult);
	printf("  - getpid me devuelve: <%d>\n", pid);
	printf("  - getppid me devuelve: <%d>\n", getppid());
	printf("  - random me devuelve: <%ld>\n", sent);
	printf("  - envío valor <%ld> a través de fd=%d\n", sent, writePipe[WRITE]);

	int writeResult = write(writePipe[WRITE], &sent, sizeof(sent));
	close(writePipe[WRITE]);

	if (writeResult < 0) {
		close(listenPipe[READ]);
		perror("Error en la escritura desde el padre\n");
		exit(-1);
	}

	long int received = 0;
	int readResult = read(listenPipe[READ], &received, sizeof(received));
	close(listenPipe[READ]);

	if (readResult < 0) {
		perror("Error en la lectura desde el padre\n");
		exit(-1);
	}

	printf("Hola, de nuevo PID <%d>:\n", pid);
	printf("  - recibí valor <%ld> vía fd=%d\n", received, listenPipe[READ]);

	if (wait(NULL) < 0) {
		perror("Error mientras esperaba al hijo");
		exit(-1);
	}
}
