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
test_0()
{
	printfmt("realloc 1:\n");
	char *var1 = realloc(NULL, 64);

	printfmt("realloc 2:\n");
	char *var2 = realloc(NULL, 64);

	printfmt("realloc 3:\n");
	char *var3 = realloc(NULL, 64);

	printfmt("realloc 4:\n");
	char *var4 = realloc(NULL, 64);

	strcpy(var2, "Probando");

	free(var1);
	free(var3);

	printfmt("realloc test:\n");
	char *var_test = realloc(var2, 180);

	printfmt("Texto: %s\n", var_test);


	free(var4);
	free(var_test);

	// free(var2);
	// free(var1);
}

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

	free(var1);
	free(var2);

	printfmt("malloc 4:\n");
	char *var4 = malloc(64 + 64 + 64);

	// strcpy(var1, "Test 1");
	strcpy(var3, "Test 3");
	strcpy(var4, "Test 4");

	// printfmt("%s\n", var1);
	printfmt("%s\n", var3);
	printfmt("%s\n", var4);

	// free(var1);

	free(var3);
	free(var3);
	free(var4);
}

void
test_2()
{
	printfmt("malloc 1:\n");
	char *var1 = malloc(32);
	printfmt("malloc 2:\n");
	char *var2 = malloc(16241);
	printfmt("malloc 3:\n");
	char *var3 = malloc(32);
	printfmt("malloc 4:\n");
	char *var4 = malloc(32);

	free(var4);
	free(var3);
	free(var2);
	free(var1);

	printfmt("malloc 5:\n");
	char *var5 = malloc(32);
	free(var5);
}

void
test_3()
{
	printfmt("malloc 1:\n");
	char *var1 = malloc(32);
	char *var_nul = NULL;

	free(((char *) var1) + 1);      // libero dentro del bloque pero corrido
	free(var_nul);                  // libero nulo
	free(((char *) var1) - 50);     // libero antes del bloque
	free(((char *) var1) + 17000);  // libero despues del bloque

	free(var1);
	free(var1);  // libero dos veces
}

void
test_4()
{
	printfmt("malloc 1:\n");
	char *var1 = malloc(64);

	printfmt("malloc 2:\n");
	char *var2 = malloc(64 * 3);

	printfmt("malloc 3:\n");
	char *var3 = malloc(64);

	printfmt("malloc 4:\n");
	char *var4 = malloc(64);

	printfmt("malloc 5:\n");
	char *var5 = malloc(64);


	free(var2);
	free(var4);

	printfmt("malloc 6:\n");
	char *var6 = malloc(64);

	free(var1);
	free(var3);
	free(var5);
	free(var6);
}


test_5()
{
	printfmt("malloc 1:\n");
	char *var1 = malloc(64);

	printfmt("malloc 6:\n");
	char *var6 = malloc(20000);

	printfmt("malloc 2:\n");
	char *var2 = malloc(64 * 3);

	printfmt("malloc 3:\n");
	char *var3 = malloc(64);

	printfmt("malloc 4:\n");
	char *var4 = malloc(64);

	printfmt("malloc 5:\n");
	char *var5 = malloc(64);


	free(var2);
	free(var4);
	free(var6);

	printfmt("malloc prueba:\n");
	char *var7 = malloc(64);

	free(var1);
	free(var3);
	free(var5);
	free(var7);
}

int
main(void)
{
	test_0();
	//   test_1();
	//    test_2();
	//    test_3();

	// test_4();
	// test_5();
	return 0;
}