#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "printfmt.h"

#define HELLO "hello from test"
#define TEST_STRING "FISOP malloc is working!"

// IMPLEMENTACION CATEDRA
//  int
//  main(void)
//  {
//  	printfmt("%s\n", HELLO);

// 	char *var = malloc(100);

// 	strcpy(var, TEST_STRING);

// 	printfmt("%s\n", var);

// 	free(var);

// 	return 0;
// }


int
main(void)
{
	// Test
	char *var1 = malloc(32);
	char *var2 = malloc(32);
	char *var3 = malloc(32);

	free(var2);
	char *var4 = malloc(100);

	strcpy(var1, "Test 1");
	strcpy(var3, "Test 3");
	strcpy(var4, "Test 4");

	printfmt("%s\n", var1);
	printfmt("%s\n", var3);
	printfmt("%s\n", var4);

	free(var1);
	free(var3);
	free(var4);

	return 0;
}