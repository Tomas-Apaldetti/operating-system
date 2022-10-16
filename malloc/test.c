#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "printfmt.h"

#include "statistics.h"

#ifndef USE_STATS
#define USE_STATS
#endif

#define KiB 1024
#define MiB (1024 * KiB)

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)

#define BLOCK_SM (16 * KiB)
#define BLOCK_MD (1 * MiB)
#define BLOCK_LG (32 * MiB)

#define MIN_REGION_LEN 64

#define REGION_HEADER_LEN 32

extern stats_t stats;
int amount_test = 0;
int amount_fail_test = 0;
int amount_ok_test = 0;


#define CLEAN_STATS                                                            \
	{                                                                      \
		YELLOW("")                                                     \
		reset_stats();                                                 \
	}

#define RUN_TEST(test)                                                         \
	{                                                                      \
		test();                                                        \
		CLEAN_STATS                                                    \
	}


#define T_RED "\x1b[31m"
#define T_GREEN "\x1b[32m"
#define T_YELLOW "\x1b[33m"
#define RESET "\x1b[0m"

#define GREEN(string) printfmt(T_GREEN "%s" RESET "\n", string);
#define RED(string) printfmt(T_RED "%s" RESET "\n", string);
#define YELLOW(string) printfmt(T_YELLOW "%s" RESET "\n", string);


//===========================================================//
//======================== SIGNATURE ========================//
//===========================================================//


//======================== MALLOC ========================//

void test_malloc(void);

void test_malloc_with_size_zero_ok(void);
void test_malloc_with_size_smaller_than_min_region_len_ok(void);
void
test_malloc_with_size_between_min_region_len_and_the_size_of_small_block_ok(void);
void test_malloc_with_size_close_to_the_size_of_small_block_ok(void);
void test_malloc_with_size_between_the_size_of_small_block_and_medium_block_ok(void);
void test_malloc_with_size_between_the_size_of_medium_block_and_large_block_ok(void);

void test_multiple_blocks_ok(void);
void test_hard_limit_error(void);

//======================== CALLOC ========================//

void test_calloc(void);

void test_calloc_with_size_zero_ok(void);
void test_calloc_with_nmemb_zero_ok(void);
void test_calloc_with_overflow_error(void);
void test_calloc_initilize_with_zeros_ok(void);

//======================== REALLOC ========================//

void test_realloc(void);

void test_realloc_with_null_pointer_behaves_as_malloc_ok(void);
void test_realloc_with_size_zero_frees_the_poninter_ok(void);
void test_realloc_with_size_zero_pointer_null(void);
void test_realloc_that_requires_less_memory(void);
void test_realloc_that_requires_less_memory_and_can_stay_and_split_inplace_even_with_next_region_being_ocuppied(
        void);
void test_realloc_that_requires_less_memory_and_can_stay_inplace_but_cannot_split_due_to_next_region_being_ocuppied_and_lack_of_space(
        void);
void test_realloc_that_requires_more_memory_and_next_region_is_free_and_enough(void);
void test_realloc_that_requires_more_memory_and_prev_region_is_free_and_enough(void);
void test_realloc_that_requires_more_memory_and_both_prev_and_next_regions_are_free_and_enough(
        void);
void test_realloc_that_requires_more_memory_and_a_new_malloc_due_to_both_prev_and_next_regions_being_occupied(
        void);
void test_realloc_that_requires_more_memory_and_a_new_malloc_due_to_both_prev_and_next_regions_being_free_but_with_lack_of_space(
        void);

//======================== FREE ========================//

void test_free(void);

void test_free_returns_memory_to_OS(void);
void test_coalesce_first_middle(void);
void test_coalesce_last_middle(void);

//======================== BEST FIT ========================//

void test_best_fit_works(void);
void test_best_fit(void);

//======================== FIRST FIT ========================//

void test_first_fit_works(void);
void test_first_fit(void);

//======================== EXTRA ========================//

void assert_ok(bool ok, char *desc);
void cpy(char *ptr, char end);
bool rev(char *ptr, char end);
void resume(void);


//===========================================================//
//======================== FUNCTIONS ========================//
//===========================================================//

//======================== EXTRA ========================//

void
assert_ok(bool ok, char *desc)
{
	amount_test++;
	if (ok) {
		GREEN(desc)
		amount_ok_test++;
	} else {
		RED(desc)
		amount_fail_test++;
	}
}

void
cpy(char *ptr, char end)
{
	char character = 0;
	for (int i = 0; i < end; i++) {
		ptr[i] = character;
		character++;
	}
}

bool
rev(char *ptr, char end)
{
	for (int i = 0; i < end; i++)
		if (ptr[i] != i)
			return false;

	return true;
}

void
resume()
{
	printfmt(T_YELLOW "===============================" RESET "\n");
	printfmt(T_YELLOW "ASSERTS: %i" RESET "\n", amount_test);

	if (amount_test == amount_ok_test)
		printfmt(T_GREEN "TESTS OK" RESET "\n");
	else
		printfmt(T_RED "ERRORES: %i" RESET "\n", amount_fail_test);

	printfmt(T_YELLOW "===============================" RESET "\n");
}

//======================== MALLOC ========================//

void
test_malloc_with_size_zero_ok()
{
	YELLOW("LLAMADA A MALLOC: size = 0")

	size_t size = 0;
	char *ptr = malloc(size);

	assert_ok(ptr == NULL,
	          "Cuando se realiza malloc con size cero devuelve nulo");

	assert_ok(stats.malloc_calls == 1,
	          "La cantidad de llamadas a malloc es uno");

	assert_ok(stats.requested_amnt == size,
	          "La cantidad de memoria pedida es cero");
	assert_ok(stats.mapped_amnt == 0,
	          "La cantidad de memoria utilizada del heap es cero");

	assert_ok(stats.curr_regions == 0,
	          "La cantidad de regiones actuales es cero");
	assert_ok(stats.curr_blocks == 0,
	          "La cantidad de bloques actuales es cero");
	assert_ok(stats.total_blocks == 0,
	          "La cantidad de total de bloques es cero");
}

void
test_malloc_with_size_smaller_than_min_region_len_ok()
{
	YELLOW("LLAMADA A MALLOC: 0 < size < MIN_REGION_LEN")

	size_t size = MIN_REGION_LEN / 2;
	char *ptr = malloc(size);

	assert_ok(ptr != NULL, "Cuando se realiza malloc con el size especificado no devuelve nulo");

	assert_ok(stats.malloc_calls == 1,
	          "La cantidad de llamadas a malloc es uno");

	assert_ok(stats.requested_amnt == size,
	          "La cantidad de memoria pedida es correcta");
	assert_ok(stats.given_amnt == MIN_REGION_LEN,
	          "La cantidad de memoria dada al usuario es correcta");
	assert_ok(stats.mapped_amnt == BLOCK_SM, "La cantidad de memoria utilizada del heap es de un bloque chico");

	assert_ok(stats.splitted_amnt == 1,
	          "La cantidad de splits realizados es uno");
	assert_ok(stats.curr_regions == 2,
	          "La cantidad de regiones actuales es dos");
	assert_ok(stats.curr_blocks == 1,
	          "La cantidad de bloques actuales es uno");
	assert_ok(stats.total_blocks == 1,
	          "La cantidad de total de bloques es uno");

	free(ptr);
}

void
test_malloc_with_size_between_min_region_len_and_the_size_of_small_block_ok()
{
	YELLOW("LLAMADA A MALLOC: MIN_REGION_LEN < size < BLOCK_SM")

	size_t size = MIN_REGION_LEN * 2;
	char *ptr = malloc(size);

	assert_ok(ptr != NULL, "Cuando se realiza malloc el size especificado no devuelve nulo");

	assert_ok(stats.malloc_calls == 1,
	          "La cantidad de llamadas a malloc es uno");

	assert_ok(stats.requested_amnt == size,
	          "La cantidad de memoria pedida es correcta");
	assert_ok(stats.given_amnt == size,
	          "La cantidad de memoria dada al usuario es correcta");
	assert_ok(stats.mapped_amnt == BLOCK_SM, "La cantidad de memoria utilizada del heap es de un bloque chico");

	assert_ok(stats.splitted_amnt == 1,
	          "La cantidad de splits realizados es uno");
	assert_ok(stats.curr_regions == 2,
	          "La cantidad de regiones actuales es dos");
	assert_ok(stats.curr_blocks == 1,
	          "La cantidad de bloques actuales es uno");
	assert_ok(stats.total_blocks == 1,
	          "La cantidad de total de bloques es uno");

	free(ptr);
}

void
test_malloc_with_size_close_to_the_size_of_small_block_ok()
{
	YELLOW("LLAMADA A MALLOC: size ≈ BLOCK_SM")

	size_t size = BLOCK_SM - REGION_HEADER_LEN;
	char *ptr = malloc(size);

	assert_ok(ptr != NULL, "Cuando se realiza malloc el size especificado no devuelve nulo");

	assert_ok(stats.malloc_calls == 1,
	          "La cantidad de llamadas a malloc es uno");

	assert_ok(stats.requested_amnt == size,
	          "La cantidad de memoria pedida es correcta");
	assert_ok(stats.given_amnt == size,
	          "La cantidad de memoria dada al usuario es correcta");
	assert_ok(stats.mapped_amnt == BLOCK_SM,
	          "La cantidad de memoria utilizada del heap es de un bloque "
	          "chico");

	assert_ok(stats.splitted_amnt == 0,
	          "La cantidad de splits realizados es cero");
	assert_ok(stats.curr_regions == 1,
	          "La cantidad de regiones actuales es uno");
	assert_ok(stats.curr_blocks == 1,
	          "La cantidad de bloques actuales es uno");
	assert_ok(stats.total_blocks == 1,
	          "La cantidad de total de bloques es uno");

	free(ptr);
}

void
test_malloc_with_size_between_the_size_of_small_block_and_medium_block_ok()
{
	YELLOW("LLAMADA A MALLOC: BLOCK_SM < size < BLOCK_MD")

	size_t size = BLOCK_SM + MIN_REGION_LEN;
	char *ptr = malloc(size);

	assert_ok(ptr != NULL, "Cuando se realiza malloc el size especificado no devuelve nulo");

	assert_ok(stats.malloc_calls == 1,
	          "La cantidad de llamadas a malloc es uno");

	assert_ok(stats.requested_amnt == size,
	          "La cantidad de memoria pedida es correcta");
	assert_ok(stats.given_amnt == size,
	          "La cantidad de memoria dada al usuario es correcta");
	assert_ok(stats.mapped_amnt == BLOCK_MD, "La cantidad de memoria utilizada del heap es de un bloque mediano");

	assert_ok(stats.splitted_amnt == 1,
	          "La cantidad de splits realizados es uno");
	assert_ok(stats.curr_regions == 2,
	          "La cantidad de regiones actuales es dos");
	assert_ok(stats.curr_blocks == 1,
	          "La cantidad de bloques actuales es uno");
	assert_ok(stats.total_blocks == 1,
	          "La cantidad de total de bloques es uno");

	free(ptr);
}

void
test_malloc_with_size_between_the_size_of_medium_block_and_large_block_ok()
{
	YELLOW("LLAMADA A MALLOC: BLOCK_MD < size < BLOCK_LG")

	size_t size = BLOCK_MD + MIN_REGION_LEN;
	char *ptr = malloc(size);

	assert_ok(ptr != NULL, "Cuando se realiza malloc el size especificado no devuelve nulo");

	assert_ok(stats.malloc_calls == 1,
	          "La cantidad de llamadas a malloc es uno");

	assert_ok(stats.requested_amnt == size,
	          "La cantidad de memoria pedida es correcta");
	assert_ok(stats.given_amnt == size,
	          "La cantidad de memoria dada al usuario es correcta");
	assert_ok(stats.mapped_amnt == BLOCK_LG, "La cantidad de memoria utilizada del heap es de un bloque grande");

	assert_ok(stats.splitted_amnt == 1,
	          "La cantidad de splits realizados es uno");
	assert_ok(stats.curr_regions == 2,
	          "La cantidad de regiones actuales es dos");
	assert_ok(stats.curr_blocks == 1,
	          "La cantidad de bloques actuales es uno");
	assert_ok(stats.total_blocks == 1,
	          "La cantidad de total de bloques es uno");

	free(ptr);
}

void
test_multiple_blocks_ok()
{
	YELLOW("LLAMADAS MULTIPLES A MALLOC")

	char *ptr1_block1;
	char *ptr2_block1;
	char *ptr3_block1;

	char *ptr1_block2;

	char *ptr1_block3;

	size_t size;

	ptr1_block1 = malloc(MIN_REGION_LEN);
	ptr2_block1 = malloc(MIN_REGION_LEN);

	ptr1_block2 = malloc(BLOCK_MD - REGION_HEADER_LEN);

	ptr1_block3 = malloc(BLOCK_MD + MIN_REGION_LEN);

	ptr3_block1 = malloc(MIN_REGION_LEN);

	size = MIN_REGION_LEN * 3 + (BLOCK_MD - REGION_HEADER_LEN) +
	       (BLOCK_MD + MIN_REGION_LEN);

	assert_ok(stats.malloc_calls == 5,
	          "La cantidad de llamadas a malloc es cinco");

	assert_ok(stats.requested_amnt == size,
	          "La cantidad de memoria pedida es correcta");
	assert_ok(stats.given_amnt == size,
	          "La cantidad de memoria dada al usuario es correcta");
	assert_ok(stats.mapped_amnt == BLOCK_LG + BLOCK_MD + BLOCK_SM,
	          "La cantidad de memoria utilizada del heap es de un "
	          "bloque "
	          "grande, un bloque mediano y un bloque chicho");

	assert_ok(stats.splitted_amnt == 4,
	          "La cantidad de splits realizados es cuatro");
	assert_ok(stats.curr_regions == 7,
	          "La cantidad de regiones actuales es siete");
	assert_ok(stats.curr_blocks == 3,
	          "La cantidad de bloques actuales es tres");
	assert_ok(stats.total_blocks == 3,
	          "La cantidad de total de bloques es tres");

	free(ptr1_block1);
	free(ptr2_block1);
	free(ptr3_block1);

	free(ptr1_block2);

	free(ptr1_block3);
}

void
test_hard_limit_error()
{
	YELLOW("LIMITE SUPERIOR DE MEMORIA EXCEDIDO")

	void *ptr_bloque1 = malloc(BLOCK_LG - REGION_HEADER_LEN);
	void *ptr_bloque2 = malloc(BLOCK_LG - REGION_HEADER_LEN);
	void *ptr_bloque3 = malloc(BLOCK_LG - REGION_HEADER_LEN);
	void *ptr_bloque4 = malloc(BLOCK_LG - REGION_HEADER_LEN);
	void *ptr_bloque5 = malloc(BLOCK_LG - REGION_HEADER_LEN);
	void *ptr_bloque6 = malloc(BLOCK_LG - REGION_HEADER_LEN);
	void *ptr_bloque7 = malloc(BLOCK_LG - REGION_HEADER_LEN);
	void *ptr_bloque8 = malloc(BLOCK_LG - REGION_HEADER_LEN);
	void *ptr_bloque9 = malloc(BLOCK_LG - REGION_HEADER_LEN);
	void *ptr_bloque10 = malloc(BLOCK_LG - REGION_HEADER_LEN);

	void *ptr_bloque11 = malloc(BLOCK_LG - REGION_HEADER_LEN);

	assert_ok(ptr_bloque11 == NULL, "El puntero resultante tras pedir memoria habiendo alcanzado el límite es nulo");
	assert_ok(errno == ENOMEM, "La variable errno está seteada en ENOMEM");

	free(ptr_bloque1);
	free(ptr_bloque2);
	free(ptr_bloque3);
	free(ptr_bloque4);
	free(ptr_bloque5);
	free(ptr_bloque6);
	free(ptr_bloque7);
	free(ptr_bloque8);
	free(ptr_bloque9);
	free(ptr_bloque10);
}

void
test_malloc()
{
	RUN_TEST(test_malloc_with_size_zero_ok)
	RUN_TEST(test_malloc_with_size_smaller_than_min_region_len_ok)

	RUN_TEST(test_malloc_with_size_between_min_region_len_and_the_size_of_small_block_ok)
	RUN_TEST(test_malloc_with_size_close_to_the_size_of_small_block_ok)
	RUN_TEST(test_malloc_with_size_between_the_size_of_small_block_and_medium_block_ok)
	RUN_TEST(test_malloc_with_size_between_the_size_of_medium_block_and_large_block_ok)

	RUN_TEST(test_multiple_blocks_ok)

	RUN_TEST(test_hard_limit_error)
}


//======================== CALLOC ========================//

void
test_calloc_with_size_zero_ok()
{
	YELLOW("LLAMADA A CALLOC CON SIZE EN CERO")

	size_t nmemb = MIN_REGION_LEN;
	size_t size = 0;

	char *ptr = calloc(nmemb, size);

	assert_ok(ptr == NULL,
	          "Cuando se realiza calloc con size cero devuelve nulo");

	assert_ok(stats.calloc_calls == 1,
	          "La cantidad de llamadas a calloc es uno");

	assert_ok(stats.requested_amnt == nmemb * size,
	          "La cantidad de memoria pedida es cero");
	assert_ok(stats.mapped_amnt == 0,
	          "La cantidad de memoria utilizada del heap es cero");

	assert_ok(stats.curr_regions == 0,
	          "La cantidad de regiones actuales es cero");
	assert_ok(stats.curr_blocks == 0,
	          "La cantidad de bloques actuales es cero");
	assert_ok(stats.total_blocks == 0,
	          "La cantidad de total de bloques es cero");
}

void
test_calloc_with_nmemb_zero_ok()
{
	YELLOW("LLAMADA A CALLOC CON NMEMb EN CERO")

	size_t nmemb = 0;
	size_t size = MIN_REGION_LEN;

	char *ptr = calloc(nmemb, size);

	assert_ok(ptr == NULL,
	          "Cuando se realiza calloc con nmemb cero devuelve nulo");

	assert_ok(stats.calloc_calls == 1,
	          "La cantidad de llamadas a calloc es uno");

	assert_ok(stats.requested_amnt == nmemb * size,
	          "La cantidad de memoria pedida es cero");
	assert_ok(stats.mapped_amnt == 0,
	          "La cantidad de memoria utilizada del heap es cero");

	assert_ok(stats.curr_regions == 0,
	          "La cantidad de regiones actuales es cero");
	assert_ok(stats.curr_blocks == 0,
	          "La cantidad de bloques actuales es cero");
	assert_ok(stats.total_blocks == 0,
	          "La cantidad de total de bloques es cero");
}

void
test_calloc_with_overflow_error()
{
	YELLOW("LLAMADA A CALLOC EN DONDE EL PRODUCTO DE SIZE Y NMEMb DA "
	       "OVERFLOW")

	size_t nmemb = __SIZE_MAX__ / 2;
	size_t size = 3;

	char *ptr = calloc(nmemb, size);

	assert_ok(ptr == NULL, "Cuando se realiza calloc en donde el producto de size y nmemb da overflow devuelve nulo");

	assert_ok(stats.calloc_calls == 1,
	          "La cantidad de llamadas a calloc es uno");

	assert_ok(stats.mapped_amnt == 0,
	          "La cantidad de memoria utilizada del heap es cero");

	assert_ok(stats.curr_regions == 0,
	          "La cantidad de regiones actuales es cero");
	assert_ok(stats.curr_blocks == 0,
	          "La cantidad de bloques actuales es cero");
	assert_ok(stats.total_blocks == 0,
	          "La cantidad de total de bloques es cero");

	assert_ok(errno == ENOMEM,
	          "La variable errno se encuentra seteada en ENOMEM");
}

void
test_calloc_initilize_with_zeros_ok()
{
	YELLOW("LLAMADA A CALLOC SETEA CORRECTAMENTE LA MEMORIA EN CERO")

	size_t nmemb = 2;
	size_t size = MIN_REGION_LEN;

	char *ptr = calloc(nmemb, size);

	bool is_memory_set = true;

	assert_ok(ptr != NULL, "Cuando se realiza calloc con parámetros válidos no devuelve nulo");

	assert_ok(stats.calloc_calls == 1,
	          "La cantidad de llamadas a calloc es uno");

	assert_ok(stats.mapped_amnt == BLOCK_SM, "La cantidad de memoria utilizada del heap es de un bloque chico");

	assert_ok(stats.curr_regions == 2,
	          "La cantidad de regiones actuales es dos");
	assert_ok(stats.curr_blocks == 1,
	          "La cantidad de bloques actuales es uno");
	assert_ok(stats.total_blocks == 1,
	          "La cantidad de total de bloques es uno");

	for (size_t i = 0; i < (nmemb * size); i++)
		if (ptr[i] != 0)
			is_memory_set = false;

	assert_ok(is_memory_set, "Cuando se realiza calloc todos los bytes se encuentran inicializados en cero");

	free(ptr);
}

void
test_calloc()
{
	RUN_TEST(test_calloc_with_size_zero_ok)
	RUN_TEST(test_calloc_with_nmemb_zero_ok)

	RUN_TEST(test_calloc_with_overflow_error)

	RUN_TEST(test_calloc_initilize_with_zeros_ok)
}


//======================== REALLOC ========================//

void
test_realloc_with_null_pointer_behaves_as_malloc_ok()
{
	YELLOW("LLAMADA A REALLOC CON NULL Y SIZE > 0");

	size_t size = 10;
	char *ptr = realloc(NULL, size);

	assert_ok(ptr != NULL, "Cuando se realiza realloc con NULL y de un size no nulo, no devuelve NULL");

	assert_ok(stats.malloc_calls == 1, "La cantidad de llamadas a malloc es uno tras hacer el realloc");

	assert_ok(stats.requested_amnt == size,
	          "La cantidad de memoria pedida es correcta");
	assert_ok(stats.given_amnt == MIN_REGION_LEN,
	          "La cantidad de memoria dada al usuario es correcta");

	assert_ok(stats.splitted_amnt == 1,
	          "La cantidad de splits realizados es uno");

	assert_ok(stats.curr_regions == 2,
	          "La cantidad de regiones actuales es dos");

	assert_ok(stats.curr_blocks == 1,
	          "La cantidad de bloques actuales es uno");

	assert_ok(stats.total_blocks == 1,
	          "La cantidad de total de bloques es uno");

	free(ptr);
}

void
test_realloc_with_size_zero_frees_the_poninter_ok()
{
	YELLOW("LLAMADA A REALLOC CON PTR NO NULL Y  SIZE = 0");

	size_t size = 10;
	char *ptr = malloc(size);
	char *aux = realloc(ptr, 0);

	assert_ok(aux == NULL, "Cuando se realiza realloc con ptr no NULL y de un size nulo, se devuelve NULL");

	assert_ok(stats.malloc_calls == 1, "La cantidad de llamadas a malloc se mantiene constante tras hacer el realloc");

	assert_ok(stats.free_calls == 1, "La cantidad de llamadas a free es uno tras hacer el realloc (por el size nulo)");

	assert_ok(stats.curr_regions == 0,
	          "La cantidad de regiones actuales es cero");

	assert_ok(stats.curr_blocks == 0,
	          "La cantidad de bloques actuales es cero");
}

void
test_realloc_with_size_zero_pointer_null()
{
	YELLOW("LLAMADA A REALLOC CON PTR NULL Y SIZE = 0");

	size_t size = 0;
	char *ptr = realloc(NULL, size);

	assert_ok(!ptr, "Cuando se realiza realloc con ptr NULL y de un size nulo, se devuelve NULL");

	assert_ok(stats.malloc_calls == 1, "La cantidad de llamadas a malloc se mantiene constante tras hacer el realloc");

	assert_ok(stats.requested_amnt == 0,
	          "La cantidad de memoria pedida es cero");

	assert_ok(stats.given_amnt == 0,
	          "La cantidad de memoria dada al usuario es cero");

	assert_ok(stats.curr_regions == 0,
	          "La cantidad de regiones actuales se mantiene en cero");

	assert_ok(stats.curr_blocks == 0,
	          "La cantidad de bloques actuales se mantiene en cero");
}

void
test_realloc_that_requires_less_memory()
{
	YELLOW("LLAMADA A REALLOC CON MENOS MEMORIA REQUERIDA QUE ANTES "
	       "(REGION UNICA)")
	size_t size_before = MIN_REGION_LEN * 2;
	size_t size_after = MIN_REGION_LEN;
	void *ptr_before = malloc(size_before);
	void *ptr_after = realloc(ptr_before, size_after);

	assert_ok(ptr_after, "Cuando se realiza el realloc, el ptr no es NULL");

	assert_ok(ptr_after == ptr_before, "El puntero apunta a la misma direccion inicial en la region retornada");

	assert_ok(stats.malloc_calls == 1, "La cantidad de llamadas a malloc se mantiene constante tras hacer el realloc");

	assert_ok(stats.requested_amnt == size_before,
	          "La cantidad de memoria pedida es correcta");

	assert_ok(stats.given_amnt == size_before,
	          "La cantidad de memoria dada al usuario es correcta");

	assert_ok(stats.splitted_amnt == 2,
	          "La cantidad de splits realizados es dos");

	assert_ok(stats.curr_regions == 2,
	          "La cantidad de regiones actuales es dos");

	assert_ok(stats.coalesced_amnt == 1,
	          "La cantidad de coalesces realizados es uno (debido al nuevo "
	          "split, para mantener la region next)");

	free(ptr_after);
}

void
test_realloc_that_requires_less_memory_and_can_stay_and_split_inplace_even_with_next_region_being_ocuppied()
{
	YELLOW("LLAMADA A REALLOC CON MENOS MEMORIA REQUERIDA QUE ANTES "
	       "(REGIONES MULTIPLES, DEBERIA MANTENER MISMO COMPORTAMIENTO)")
	size_t size_before = MIN_REGION_LEN * 3;
	size_t size_after = MIN_REGION_LEN;
	void *ptr_before = malloc(size_before);
	void *ptr_next = malloc(size_before);
	void *ptr_after = realloc(ptr_before, size_after);

	assert_ok(ptr_after, "Cuando se realiza el realloc, el ptr no es NULL");

	assert_ok(ptr_after == ptr_before, "El puntero apunta a la misma direccion inicial en la region retornada");

	assert_ok(stats.malloc_calls == 2, "La cantidad de llamadas a malloc se mantiene constante tras hacer el realloc");

	assert_ok(stats.requested_amnt == size_before * 2,
	          "La cantidad de memoria pedida es correcta");

	assert_ok(stats.splitted_amnt == 3,
	          "La cantidad de splits realizados es tres");

	assert_ok(stats.curr_regions == 4,
	          "La cantidad de regiones actuales es cuatro");
	// Regiones:  | ocupada tras realloc | libre por split | ocupada por 2do malloc | libre ...|

	assert_ok(stats.coalesced_amnt == 0,
	          "La cantidad de coalesces realizados es cero (a pesar de "
	          "tener nuevo split, ya que region next estaba ocupada)");

	free(ptr_after);
	free(ptr_next);
}

void
test_realloc_that_requires_less_memory_and_can_stay_inplace_but_cannot_split_due_to_next_region_being_ocuppied_and_lack_of_space()
{
	YELLOW("LLAMADA A REALLOC CON MENOS MEMORIA REQUERIDA QUE ANTES "
	       "(REGIONES MULTIPLES, PERO SIN SPLIT ADICIONAL)")
	size_t size_before = MIN_REGION_LEN * 2;
	size_t size_after =
	        MIN_REGION_LEN;  // Pidiendo esta cantidad no se tiene suficiente
	                         // para splittear ya que no sobraría para otra region + su header.
	void *ptr_before = malloc(size_before);
	void *ptr_next = malloc(size_before);
	void *ptr_after = realloc(ptr_before, size_after);

	assert_ok(ptr_after, "Cuando se realiza el realloc, el ptr no es NULL");

	assert_ok(ptr_after == ptr_before, "El puntero apunta a la misma direccion inicial en la region retornada");

	assert_ok(stats.malloc_calls == 2, "La cantidad de llamadas a malloc se mantiene constante tras hacer el realloc");

	assert_ok(stats.requested_amnt == size_before * 2,
	          "La cantidad de memoria pedida es correcta");

	assert_ok(stats.splitted_amnt == 2,
	          "La cantidad de splits realizados es dos");

	assert_ok(stats.curr_regions == 3,
	          "La cantidad de regiones actuales es tres");
	// Regiones:  | ocupada tras realloc | ocupada por 2do malloc | libre ...|

	assert_ok(stats.coalesced_amnt == 0,
	          "La cantidad de coalesces realizados es cero");

	free(ptr_after);
	free(ptr_next);
}

void
test_realloc_that_requires_more_memory_and_next_region_is_free_and_enough()
{
	YELLOW("LLAMADA A REALLOC CON MAS MEMORIA REQUERIDA QUE ANTES (REGION "
	       "ADYACENTE A LA DERECHA LIBRE Y SUFICIENTE)")
	size_t size_before = MIN_REGION_LEN;
	size_t size_after = MIN_REGION_LEN * 2;
	void *ptr_before = malloc(size_before);
	void *ptr_after = realloc(ptr_before, size_after);

	assert_ok(ptr_after, "Cuando se realiza el realloc, el ptr no es NULL");

	assert_ok(ptr_before == ptr_after, "El puntero apunta a la misma direccion inicial en la region retornada");

	assert_ok(stats.requested_amnt == size_after,
	          "La cantidad de memoria pedida es correcta");

	assert_ok(stats.curr_regions == 2,
	          "La cantidad de regiones actuales es dos");

	assert_ok(stats.coalesced_amnt == 1,
	          "La cantidad de coalesces realizados es uno");

	assert_ok(stats.realloc_optimized == 1,
	          "El realloc esta optimizado para no requerir otro malloc");

	free(ptr_after);
}

void
test_realloc_that_requires_more_memory_and_prev_region_is_free_and_enough()
{
	YELLOW("LLAMADA A REALLOC CON MAS MEMORIA REQUERIDA QUE ANTES (REGION "
	       "ADYACENTE A LA IZQUIERDA LIBRE Y SUFICIENTE)")
	int size_before = MIN_REGION_LEN;
	int size_after = MIN_REGION_LEN * 2;
	char *ptr_prev = malloc(size_before);
	char *ptr_before = malloc(size_before);
	char *ptr_next = malloc(size_before);

	cpy(ptr_before, MIN_REGION_LEN);

	free(ptr_prev);

	void *ptr_after = realloc(ptr_before, size_after);

	assert_ok(ptr_after, "Cuando se realiza el realloc, el ptr no es NULL");

	assert_ok(ptr_before != ptr_after, "El puntero apunta a una direccion inicial distinta en la region retornada");

	assert_ok(stats.requested_amnt == MIN_REGION_LEN * 4,
	          "La cantidad de memoria pedida es correcta");

	assert_ok(stats.curr_regions == 3, "La cantidad de regiones es tres");
	// Regiones:  | ocupada tras realloc | ocupada por 3er malloc | libre ... |

	assert_ok(stats.coalesced_amnt == 1,
	          "La cantidad de coalesces realizados es uno");

	assert_ok(stats.realloc_optimized == 1,
	          "El realloc esta optimizado para no requerir otro malloc");

	assert_ok(rev(ptr_after, MIN_REGION_LEN), "La data alamacenada tras el realloc en las nuevas direcciones es correcta");

	free(ptr_after);
	free(ptr_next);
}

void
test_realloc_that_requires_more_memory_and_both_prev_and_next_regions_are_free_and_enough()
{
	YELLOW("LLAMADA A REALLOC CON MAS MEMORIA REQUERIDA QUE ANTES (AMBAS "
	       "REGIONES ADYACENTES LIBRES Y SUFICIENTES)")
	int size_before = MIN_REGION_LEN;
	int size_after = MIN_REGION_LEN * 3;
	char *ptr_prev = malloc(size_before);
	char *ptr_before = malloc(size_before);
	char *ptr_next = malloc(size_before);
	char *ptr_nnext = malloc(size_before);

	cpy(ptr_before, MIN_REGION_LEN);

	free(ptr_prev);
	free(ptr_next);
	void *ptr_after = realloc(ptr_before, size_after);

	assert_ok(ptr_after, "Cuando se realiza el realloc, el ptr no es NULL");

	assert_ok(ptr_before != ptr_after, "El puntero apunta a una direccion inicial distinta en la region retornada");

	assert_ok(stats.requested_amnt == MIN_REGION_LEN * 6,
	          "La cantidad de memoria pedida es correcta");

	assert_ok(stats.curr_regions == 3, "La cantidad de regiones es tres");
	// Regiones:  | ocupada tras realloc | ocupada por 4to malloc | libre ... |

	assert_ok(stats.coalesced_amnt == 2,
	          "La cantidad de coalesces realizados es dos");

	assert_ok(stats.realloc_optimized == 1,
	          "El realloc esta optimizado para no requerir otro malloc");

	assert_ok(rev(ptr_after, MIN_REGION_LEN), "La data alamacenada tras el realloc en las nuevas direcciones es correcta");

	free(ptr_after);
	free(ptr_nnext);
}

void
test_realloc_that_requires_more_memory_and_a_new_malloc_due_to_both_prev_and_next_regions_being_occupied()
{
	YELLOW("LLAMADA A REALLOC CON MAS MEMORIA REQUERIDA QUE ANTES (AMBAS "
	       "REGIONES ADYACENTES OCUPADAS)");
	int size_before = MIN_REGION_LEN;
	int size_after = MIN_REGION_LEN * 3;
	char *ptr_prev = malloc(size_before);
	char *ptr_before = malloc(size_before);
	char *ptr_next = malloc(size_before);

	cpy(ptr_before, MIN_REGION_LEN);

	void *ptr_after = realloc(ptr_before, size_after);

	assert_ok(ptr_after, "Cuando se realiza el realloc, el ptr no es NULL");

	assert_ok(ptr_before != ptr_after, "El puntero apunta a una direccion inicial distinta en la region retornada");

	assert_ok(stats.malloc_calls == 4,
	          "La cantidad de llamados a malloc es cuatro");

	assert_ok(stats.free_calls == 1, "La cantidad de llamadas a free es uno");

	assert_ok(stats.freed_amnt == MIN_REGION_LEN,
	          "La cantidad de memoria liberada es correcta");

	assert_ok(stats.curr_regions == 5, "La cantidad de regiones es cinco");

	assert_ok(stats.coalesced_amnt == 0,
	          "La cantidad de coalesces realizados es cero");

	assert_ok(stats.realloc_no_optimized == 1,
	          "El realloc no esta optimizado pues requiere otro malloc");

	assert_ok(rev(ptr_after, MIN_REGION_LEN), "La data alamacenada tras el realloc en las nuevas direcciones es correcta");

	free(ptr_prev);
	free(ptr_next);
	free(ptr_after);
}

void
test_realloc_that_requires_more_memory_and_a_new_malloc_due_to_both_prev_and_next_regions_being_free_but_with_lack_of_space()
{
	YELLOW("LLAMADA A REALLOC CON MAS MEMORIA REQUERIDA QUE ANTES (AMBAS "
	       "REGIONES ADYACENTES LIBRES PERO SIN SUFICIENTE ESPACIO)");
	int size_before = MIN_REGION_LEN;
	int size_after = MIN_REGION_LEN * 8;
	char *ptr_pprev = malloc(size_before);
	char *ptr_prev = malloc(size_before);
	char *ptr_before = malloc(size_before);
	char *ptr_next = malloc(size_before);
	char *ptr_nnext = malloc(size_before);

	cpy(ptr_before, MIN_REGION_LEN);

	free(ptr_prev);
	free(ptr_next);

	void *ptr_after = realloc(ptr_before, size_after);

	assert_ok(ptr_after, "Cuando se realiza el realloc, el ptr no es NULL");

	assert_ok(ptr_before != ptr_after, "El puntero apunta a una direccion inicial distinta en la region retornada");

	assert_ok(stats.malloc_calls == 6,
	          "La cantidad de llamados a malloc es seis");

	assert_ok(stats.free_calls == 3, "La cantidad de llamadas a free es tres");

	assert_ok(stats.freed_amnt == MIN_REGION_LEN * 3,
	          "La cantidad de memoria liberada es correcta");

	assert_ok(stats.curr_regions == 5, "La cantidad de regiones es cinco");
	// Regiones:  | ptr_pprev | ... libre (tras dos coalesces por el free) ... | ptr_nnext | ptr_after | libre ... |

	assert_ok(stats.coalesced_amnt == 2,
	          "La cantidad de coalesces realizados es dos");

	assert_ok(stats.splitted_amnt == 6,
	          "La cantidad de splits realizados es seis");

	assert_ok(stats.realloc_no_optimized == 1,
	          "El realloc no esta optimizado pues requiere otro malloc");

	assert_ok(rev(ptr_after, MIN_REGION_LEN), "La data alamacenada tras el realloc en las nuevas direcciones es correcta");

	free(ptr_pprev);
	free(ptr_nnext);
	free(ptr_after);
}

void
test_realloc()
{
	RUN_TEST(test_realloc_with_null_pointer_behaves_as_malloc_ok)
	RUN_TEST(test_realloc_with_size_zero_frees_the_poninter_ok)

	RUN_TEST(test_realloc_with_size_zero_pointer_null)
	RUN_TEST(test_realloc_that_requires_less_memory)
	RUN_TEST(test_realloc_that_requires_less_memory_and_can_stay_and_split_inplace_even_with_next_region_being_ocuppied)
	RUN_TEST(test_realloc_that_requires_less_memory_and_can_stay_inplace_but_cannot_split_due_to_next_region_being_ocuppied_and_lack_of_space)
	RUN_TEST(test_realloc_that_requires_more_memory_and_next_region_is_free_and_enough)
	RUN_TEST(test_realloc_that_requires_more_memory_and_prev_region_is_free_and_enough)
	RUN_TEST(test_realloc_that_requires_more_memory_and_both_prev_and_next_regions_are_free_and_enough)
	RUN_TEST(test_realloc_that_requires_more_memory_and_a_new_malloc_due_to_both_prev_and_next_regions_being_occupied)
	RUN_TEST(test_realloc_that_requires_more_memory_and_a_new_malloc_due_to_both_prev_and_next_regions_being_free_but_with_lack_of_space)
}


//======================== FREE ========================//

void
test_free_returns_memory_to_OS()
{
	YELLOW("LLAMADA A FREE LIBERA BLOQUE ENTERO Y SE DEVUELVE AL SISTEMA")
	char *ptr = malloc(150);
	free(ptr);

	assert_ok(stats.free_calls == 1, "La cantidad de free calls es correcta");

	assert_ok(stats.freed_amnt == ALIGN4(150),
	          "La cantidad de memoria liberada es correcta");

	assert_ok(stats.curr_blocks == 0,
	          "La cantidad de bloques actuales es cero");

	assert_ok(stats.coalesced_amnt == 1,
	          "La cantidad de coalesces realizados es uno");

	assert_ok(stats.returned_blocks == 1, "Se devolvio un bloque al sistema");
}

void
test_coalesce_first_middle()
{
	YELLOW("LLAMADO A FREE REALIZA COALESCE CON REGIONES ADYACENTES (ORDEN "
	       "DE LIBERACION DE REGIONES: INTERMEDIA --> DERECHA --> "
	       "IZQUIERDA)")
	char *ptr1 = malloc(150);
	char *ptr2 = malloc(150);
	char *ptr3 = malloc(150);


	free(ptr2);

	assert_ok(stats.free_calls == 1, "La cantidad de free calls es 1");
	assert_ok(stats.freed_amnt == ALIGN4(150),
	          "La cantidad de memoria liberada es correcta");
	assert_ok(stats.coalesced_amnt == 0,
	          "La cantidad de coalesces realizados es cero\n");

	free(ptr3);
	assert_ok(stats.free_calls == 2, "La cantidad de free calls es 2");
	assert_ok(stats.freed_amnt == ALIGN4(150) * 2,
	          "La cantidad de memoria liberada es correcta");
	assert_ok(stats.coalesced_amnt == 2,
	          "La cantidad de coalesces realizados es dos");
	assert_ok(stats.curr_regions == 2,
	          "La cantidad de regiones es correcta luego del coalesce\n");

	free(ptr1);
	assert_ok(stats.free_calls == 3, "La cantidad de free calls es 3");
	assert_ok(stats.freed_amnt == ALIGN4(150) * 3,
	          "La cantidad de memoria liberada es correcta");
	assert_ok(stats.coalesced_amnt == 3,
	          "La cantidad de coalesces realizados es tres");
}

void
test_coalesce_last_middle()
{
	YELLOW("LLAMADO A FREE REALIZA COALESCE CON REGIONES ADYACENTES (ORDEN "
	       "DE LIBERACION DE REGIONES: IZQUIERDA --> DERECHA --> "
	       "INTERMEDIA)")
	char *ptr1 = malloc(150);
	char *ptr2 = malloc(150);
	char *ptr3 = malloc(150);

	free(ptr1);
	free(ptr3);

	assert_ok(stats.free_calls == 2, "La cantidad de free calls es 2");
	assert_ok(stats.freed_amnt == ALIGN4(150) * 2,
	          "La cantidad de memoria liberada es correcta");
	assert_ok(stats.coalesced_amnt == 1,
	          "La cantidad de coalesces realizados es uno");
	assert_ok(stats.curr_regions == 3,
	          "La cantidad de regiones es correcta luego del coalesce\n");

	free(ptr2);
	assert_ok(stats.free_calls == 3, "La cantidad de free calls es 2");
	assert_ok(stats.freed_amnt == ALIGN4(150) * 3,
	          "La cantidad de memoria liberada es correcta");
	assert_ok(stats.coalesced_amnt == 3,
	          "La cantidad de coalesces realizados es uno");
}

void
test_free()
{
	RUN_TEST(test_free_returns_memory_to_OS)

	RUN_TEST(test_coalesce_first_middle)

	RUN_TEST(test_coalesce_last_middle)
}


//======================== BEST FIT ========================//

void
test_best_fit_works()
{
	YELLOW("TEST BEST FIT")
	size_t size = MIN_REGION_LEN * 2;
	void *ptr1 = malloc(size);
	void *ptr2 = malloc(size * 2);
	void *ptr3 = malloc(size);
	void *ptr4 = malloc(size);
	void *ptr5 = malloc(size);
	void *ptr6 = malloc(size);
	free(ptr2);
	free(ptr4);
	void *ptr7 = malloc(size);
	assert_ok(ptr7 == ptr4, "Punteros son iguales");
	free(ptr1);
	free(ptr3);
	free(ptr5);
	free(ptr6);
	free(ptr7);
}

void
test_best_fit()
{
	RUN_TEST(test_best_fit_works)
}


//======================== FIRST FIT ========================//

void
test_first_fit_works()
{
	YELLOW("TEST FIRST FIT")
	size_t size = MIN_REGION_LEN * 2;
	void *ptr1 = malloc(size);
	void *ptr2 = malloc(size * 2);
	void *ptr3 = malloc(size);
	void *ptr4 = malloc(size);
	void *ptr5 = malloc(size);
	void *ptr6 = malloc(size);
	free(ptr2);
	free(ptr4);
	void *ptr7 = malloc(size);
	assert_ok(ptr7 == ptr2, "Punteros son iguales");
	free(ptr1);
	free(ptr3);
	free(ptr5);
	free(ptr6);
	free(ptr7);
}

void
test_first_fit()
{
	RUN_TEST(test_first_fit_works)
}


//======================================================//
//======================== MAIN ========================//
//======================================================//

int
main()
{
#ifndef USE_STATS
	printfmt("No se pueden ejecutar los tests sin definir USE_STATS");
	return 0;
#endif

	test_malloc();

	test_calloc();

	test_realloc();

	test_free();

#ifdef BEST_FIT
	test_best_fit();
#endif
#ifdef FIRST_FIT
	test_first_fit();
#endif

	resume();
	return 0;
}
