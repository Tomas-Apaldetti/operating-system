#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "printfmt.h"

#define KiB 1024
#define MiB (1024 * KiB)

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)

#define BLOCK_SM (16 * KiB)
#define BLOCK_MD (1 * MiB)
#define BLOCK_LG (32 * MiB)

#define MIN_REGION_LEN 64

#ifdef USE_STATS
#include "statistics.h"
extern stats_t stats;
#define CLEAN_STATS                                                            \
	{                                                                      \
		YELLOW("")                                                     \
		reset_stats();                                                 \
	}

#define RUN_TEST(test) test();
#else
#define CLEAN_STATS
#define RUN_TEST(test) printfmt("Can't run test without statistics defined");
#endif

#define T_RED "\x1b[31m"
#define T_GREEN "\x1b[32m"
#define T_YELLOW "\x1b[33m"
#define RESET "\x1b[0m"

#define GREEN(string) printfmt(T_GREEN "%s" RESET "\n", string);
#define RED(string) printfmt(T_RED "%s" RESET "\n", string);
#define YELLOW(string) printfmt(T_YELLOW "%s" RESET "\n", string);

void
assert_ok(bool ok, char *desc)
{
	if (ok)
		GREEN(desc)
	else
		RED(desc)
}


// ======================== MALLOC ======================== //

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
test_malloc_with_size_between_min_region_len_and_the_size_of_small_block()
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
test_malloc_with_size_close_to_the_size_of_small_block()
{
	YELLOW("LLAMADA A MALLOC: size ≈ BLOCK_SM")

	size_t size = BLOCK_SM - 40;
	char *ptr = malloc(size);

	assert_ok(ptr != NULL, "Cuando se realiza malloc el size especificado no devuelve nulo");

	assert_ok(stats.malloc_calls == 1,
	          "La cantidad de llamadas a malloc es uno");

	assert_ok(stats.requested_amnt == size,
	          "La cantidad de memoria pedida es correcta");
	assert_ok(stats.given_amnt == size,
	          "La cantidad de memoria dada al usuario es correcta");
	assert_ok(stats.mapped_amnt == BLOCK_SM, "La cantidad de memoria utilizada del heap es de un bloque chico");

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
test_malloc_with_size_between_the_size_of_small_block_and_medium_block()
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
test_malloc_with_size_between_the_size_of_medium_block_and_large_block()
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
test_multiple_blocks()
{
	YELLOW("LLAMADAs MULTIPLES A MALLOC")

	char *ptr1_block1;
	char *ptr2_block1;
	char *ptr3_block1;

	char *ptr1_block2;

	char *ptr1_block3;

	size_t size;

	ptr1_block1 = malloc(MIN_REGION_LEN);
	ptr2_block1 = malloc(MIN_REGION_LEN);
	ptr1_block2 = malloc(BLOCK_MD - 40);
	ptr1_block3 = malloc(BLOCK_MD + MIN_REGION_LEN);
	ptr3_block1 = malloc(MIN_REGION_LEN);

	size = MIN_REGION_LEN * 3 + (BLOCK_MD - 40) + (BLOCK_MD + MIN_REGION_LEN);


	assert_ok(stats.malloc_calls == 5,
	          "La cantidad de llamadas a malloc es cinco");

	assert_ok(stats.requested_amnt == size,
	          "La cantidad de memoria pedida es correcta");
	assert_ok(stats.given_amnt == size,
	          "La cantidad de memoria dada al usuario es correcta");
	assert_ok(stats.mapped_amnt == BLOCK_LG + BLOCK_MD + BLOCK_SM,
	          "La cantidad de memoria utilizada del heap es de un bloque "
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
test_malloc()
{
	RUN_TEST(test_malloc_with_size_zero_ok)
	CLEAN_STATS

	RUN_TEST(test_malloc_with_size_smaller_than_min_region_len_ok)
	CLEAN_STATS

	RUN_TEST(test_malloc_with_size_between_min_region_len_and_the_size_of_small_block)
	CLEAN_STATS

	RUN_TEST(test_malloc_with_size_close_to_the_size_of_small_block)
	CLEAN_STATS

	RUN_TEST(test_malloc_with_size_between_the_size_of_small_block_and_medium_block)
	CLEAN_STATS

	RUN_TEST(test_malloc_with_size_between_the_size_of_medium_block_and_large_block)
	CLEAN_STATS

	RUN_TEST(test_multiple_blocks)
	CLEAN_STATS
}


// ======================== CALLOC ======================== //

void
test_calloc_contains_zero()
{
	YELLOW("LLAMADA A CALLOC CONTAINS ZERO")
	char *ptr = calloc(1, 150);

	bool ok = true;
	for (int i = 0; i < ALIGN4(150); i++) {
		if (ptr[i] != 0)
			ok = false;
	}

	assert_ok(ok, "Calloc de 150 es puro 0");
	free(ptr);
}

void
test_calloc()
{
	RUN_TEST(test_calloc_contains_zero)
	CLEAN_STATS
}


// ======================== REALLOC ======================== //

void
test_realloc_with_null_pointer_behaves_as_malloc_ok()
{
	YELLOW("LLAMADA A REALLOC CON NULL Y TAMAÑO > 0");

	size_t size = 10;
	char *ptr = realloc(NULL, size);

	assert_ok(ptr != NULL, "Cuando se realiza realloc con NULL y de un size no nulo, no devuelve NULL");

	assert_ok(stats.malloc_calls == 1, "La cantidad de llamadas a malloc es uno tras hacer el realloc");

	assert_ok(stats.requested_amnt == size,
	          "La cantidad de memoria pedida es correcta");
	assert_ok(stats.given_amnt == MIN_REGION_LEN,
	          "La cantidad de memoria dada al usuario es correcta");
	assert_ok(stats.mapped_amnt == BLOCK_SM, "La cantidad de memoria utilizada del heap es de un bloque pequeño");

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
	YELLOW("LLAMADA A REALLOC CON TAMAÑO CERO (PTR NO NULL)");

	size_t size = 10;
	char *ptr = malloc(size);

	char *aux = realloc(ptr, 0);

	assert_ok(ptr != NULL, "Cuando se realiza realloc con ptr no NULL y de un size nulo, se devuelve NULL");

	assert_ok(stats.malloc_calls == 1, "La cantidad de llamadas a malloc se mantiene constante tras hacer el realloc");

	assert_ok(
	        stats.free_calls == 1,
	        "La cantidad de llamadas a free es uno tras hacer el realloc");

	assert_ok(stats.curr_blocks == 0,
	          "La cantidad de bloques actuales es cero");

	assert_ok(stats.total_blocks == 1,
	          "La cantidad de total de bloques es uno");
}

/*
void
test_realloc_with_null_pointer_and_size_zero_returns_null_ok()
{
        YELLOW("");

        assert_ok(true, "");
}

void
test_realloc_of_larger_size_than_prev_mem_is_handled_ok()
{
        YELLOW("");

        assert_ok(true, "");
}

void
test_realloc_of_smaller_size_than_prev_mem_is_handled_ok()
{
        YELLOW("");

        assert_ok(true, "");
}

void
test_realloc_of_equal_size_as_prev_mem_returns_the_same_ptr_ok()
{
        YELLOW("");

        assert_ok(true, "");
}
*/

void
test_realloc()
{
	RUN_TEST(test_realloc_with_null_pointer_behaves_as_malloc_ok)
	CLEAN_STATS

	RUN_TEST(test_realloc_with_size_zero_frees_the_poninter_ok)
	CLEAN_STATS

	// RUN_TEST(test_realloc_with_null_pointer_and_size_zero_returns_null_ok)
	// CLEAN_STATS

	// RUN_TEST(test_realloc_of_larger_size_than_prev_mem_is_handled_ok)
	// CLEAN_STATS

	// RUN_TEST(test_realloc_of_smaller_size_than_prev_mem_is_handled_ok)
	// CLEAN_STATS

	// RUN_TEST(test_realloc_of_equal_size_as_prev_mem_returns_the_same_ptr_ok)
	// CLEAN_STATS
}


// ======================== FREE ======================== //

void
test_return_mmy_to_OS()
{
	YELLOW("LIBERO BLOQUE Y SE DEVUELVE AL SISTEMA")
	char *ptr = malloc(150);
	free(ptr);

	assert_ok(stats.free_calls == 1, "Cantidad de free calls es correcta");
	assert_ok(stats.freed_amnt == ALIGN4(150),
	          "Cantidad de memoria liberada es correcta");
	assert_ok(stats.curr_blocks == 0, "No hay bloques actuales");
	assert_ok(stats.coalesced_amnt == 1, "Se realizo un coalesce");
	assert_ok(stats.total_blocks == 1,
	          "Se pidio un bloque antes de liberarlo");
	assert_ok(stats.returned_blocks == 1, "Se devolvio un bloque");
}

void
test_coalesce_first_middle()
{
	YELLOW("TEST COALESCE FIRST MIDDLE")
	char *ptr1 = malloc(150);
	char *ptr2 = malloc(150);
	char *ptr3 = malloc(150);

	YELLOW("Libero el bloque de en medio")
	free(ptr2);

	assert_ok(stats.free_calls == 1, "Cantidad de free calls es 1");
	assert_ok(stats.freed_amnt == ALIGN4(150),
	          "Cantidad de memoria liberada es correcta");
	assert_ok(stats.coalesced_amnt == 0, "No se realizo coalesce");

	YELLOW("Libero el bloque de la izquierda")
	free(ptr3);
	assert_ok(stats.free_calls == 2, "Cantidad de free calls es 2");
	assert_ok(stats.freed_amnt == ALIGN4(150) * 2,
	          "Cantidad de memoria liberada es correcta");
	assert_ok(stats.coalesced_amnt == 2, "Se realizo dos coalesce");
	assert_ok(stats.curr_regions == 2,
	          "La cantidad de regiones es correcta luego del coalesce");

	YELLOW("Libero el bloque de la derecha")
	free(ptr1);
	assert_ok(stats.free_calls == 3, "Cantidad de free calls es 3");
	assert_ok(stats.freed_amnt == ALIGN4(150) * 3,
	          "Cantidad de memoria liberada es correcta");
	assert_ok(stats.coalesced_amnt == 3, "Se realizo tres coalesce");
}

void
test_coalesce_last_middle()
{
	YELLOW("TEST COALESCE LAST MIDDLE")
	char *ptr1 = malloc(150);
	char *ptr2 = malloc(150);
	char *ptr3 = malloc(150);

	YELLOW("Libero los bloques adyacentes")
	free(ptr1);
	free(ptr3);

	assert_ok(stats.free_calls == 2, "Cantidad de free calls es 2");
	assert_ok(stats.freed_amnt == ALIGN4(150) * 2,
	          "Cantidad de memoria liberada es correcta");
	assert_ok(stats.coalesced_amnt == 1, "Se realizo un coalesce");
	assert_ok(stats.curr_regions == 3,
	          "La cantidad de regiones es correcta luego del coalesce");

	YELLOW("Libero bloque del medio")
	free(ptr2);
	assert_ok(stats.free_calls == 3, "Cantidad de free calls es 2");
	assert_ok(stats.freed_amnt == ALIGN4(150) * 3,
	          "Cantidad de memoria liberada es correcta");
	assert_ok(stats.coalesced_amnt == 3, "Se realizo un coalesce");
}

void
test_free()
{
	RUN_TEST(test_return_mmy_to_OS)
	CLEAN_STATS

	RUN_TEST(test_coalesce_first_middle)
	CLEAN_STATS

	RUN_TEST(test_coalesce_last_middle)
	CLEAN_STATS
}


// ======================== BEST FIT STRAT ======================== //

void
test_best_fit()
{
	YELLOW("TO BE IMPLEMENTED");
}


// ======================== FIRST FIT STRAT ======================== //

void
test_first_fit()
{
	YELLOW("TO BE IMPLEMENTED");
}


// ==================================================== //

int
main(void)
{
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

	return 0;
}
