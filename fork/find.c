#define _GNU_SOURCE
#define MAX_STR_SIZE 2048

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

typedef char *(*Contains)(const char *haystack, const char *needle);

Contains caseSensitiveFlag(char *argv[], char **searchTerm);

int find(char *searchTerm, Contains contains);

int findInDir(DIR *dir,
              char *searchTerm,
              Contains contains,
              char dirName[MAX_STR_SIZE]);

void formDir(char destiny[MAX_STR_SIZE], char *base, char *toAdd);

int
main(int argc, char *argv[])
{
	if (argc < 2 || argc > 3) {
		printf("La cantidad de argumentos es incorrecta");
		exit(0);
	}

	char *searchTerm;
	Contains contains = caseSensitiveFlag(argv, &searchTerm);

	if (find(searchTerm, contains) != 0) {
		fprintf(stderr, "Hubo un error al realizar la busqueda en el directorio\n");
		exit(-1);
	}
}

Contains
caseSensitiveFlag(char *argv[], char **searchTerm)
{
	if (strcmp(argv[1], "-i") == 0) {
		*searchTerm = argv[2];
		return strcasestr;
	}
	*searchTerm = argv[1];
	return strstr;
}

int
find(char *searchTerm, Contains contains)
{
	errno = 0;
	DIR *baseDir = opendir(".");
	if (!baseDir || errno != 0) {
		perror("Error al abrir el directorio base");
		return -1;
	}
	char base[MAX_STR_SIZE];
	strcpy(base, ".");
	int result = findInDir(baseDir, searchTerm, contains, base);
	if (closedir(baseDir) < 0) {
		perror("Error al cerrar el directorio");
		return ++result;
	}
	return result;
}

int
findInDir(DIR *dir, char *searchTerm, Contains contains, char dirName[MAX_STR_SIZE])
{
	int errAmnt = 0;
	struct dirent *entry;
	errno = 0;
	while ((entry = readdir(dir))) {
		
		if (strcmp(entry->d_name, ".") == 0 ||
		    strcmp(entry->d_name, "..") == 0)
			continue;

		char complete[2048];
		formDir(complete, dirName, entry->d_name);
		if (contains(entry->d_name, searchTerm))
			printf("%s\n", complete);

		if (entry->d_type == DT_DIR) {
			int result = openat(dirfd(dir), entry->d_name, 0);
			if (result < 0) {
				perror("Error al leer un directorio");
				errno = 0;
				errAmnt++;
			} else {
				errAmnt += findInDir(fdopendir(result),
				                     searchTerm,
				                     contains,
				                     complete);
			}
		}
	}

	if (errno != 0) {
		perror("Error al leer un archivo del del directorio");
		errAmnt++;
	}

	return errAmnt;
}

void
formDir(char destiny[MAX_STR_SIZE], char *base, char *toAdd)
{
	strcpy(destiny, base);
	strcat(destiny, "/");
	strcat(destiny, toAdd);
}