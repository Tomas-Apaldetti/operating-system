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

void
test_1()
{
	// Test
	printfmt("malloc 1:\n");
	char *var1 = malloc(32);
	printfmt("malloc 2:\n");
	char *var2 = malloc(32);
	printfmt("malloc 3:\n");
	char *var3 = malloc(32);

	free(var2);
	free(var1);

	printfmt("malloc 4:\n");

	char *var4 = malloc(64);

	// strcpy(var1, "Test 1");
	strcpy(var3, "Test 3");
	strcpy(var4, "Test 4");

	// printfmt("%s\n", var1);
	printfmt("%s\n", var3);
	printfmt("%s\n", var4);

	// free(var1);

	free(var3);
	free(var4);
}

void
test_2()
{
	char *var1 = malloc(32);
	char *var2 = malloc(32);
	char *var3 = malloc(32);

	char *var4 = malloc(20000);

	free(var4);

	char *var5 = malloc(100);

	free(var3);
	free(var2);
	free(var5);
	free(var1);
}

int
main(void)
{
	// test_1();
	test_2();

	return 0;
}