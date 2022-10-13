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

#ifdef USE_STATS
#include "statistics.h"
stats_t stats;
#define CLEAN_STATS { 			\
	YELLOW("LIMPIANDO STATS")	\
	reset_stats();				\
}

#define RUN_TEST(test) test();
#else
#define CLEAN_STATS
#define RUN_TEST(test) printfmt("Can't run test without statistics defined");                                                       
#endif

#define T_RED     "\x1b[31m"
#define T_GREEN   "\x1b[32m"
#define T_YELLOW  "\x1b[33m"
#define RESET "\x1b[0m"

#define GREEN(string) printfmt(T_GREEN "%s" RESET "\n", string);
#define RED(string) printfmt(T_RED "%s" RESET "\n", string);
#define YELLOW(string) printfmt(T_YELLOW "%s" RESET "\n", string);

void
assert_ok(bool ok, char* desc){
	if(ok)
		GREEN(desc)
	else
		RED(desc)
}

void
test_malloc_with_zero(){

	YELLOW("LLAMADA A MALLOC CON 0")
	char* isNull = malloc(0);
	assert_ok(!isNull, "Malloc con 0 es null");
	assert_ok(stats.malloc_calls == 1, "Llamadas a malloc es 1");
	assert_ok(stats.requested_amnt == 0, "Cantidad pedida es 0");
	assert_ok(stats.curr_blocks == 0, "Cantidad de bloques es 0");
	assert_ok(stats.total_blocks == 0, "Cantidad total de bloques es 0");
	assert_ok(stats.curr_regions == 0, "Cantidad de regiones es 0");
	assert_ok(stats.mapped_amnt == 0, "Tamanio del heap es 0");
}

void
test_malloc_lt_64(){
	YELLOW("LLAMADA A MALLOC CON 32")
	char* ptr = malloc(32);
	assert_ok(ptr, "Malloc con 32 no es null");
	assert_ok(stats.malloc_calls == 1, "Llamadas a malloc es 1");
	assert_ok(stats.requested_amnt == 32, "Cantidad pedida es 32");
	assert_ok(stats.given_amnt == 64, "Cantidad recibida es 64");
	assert_ok(stats.curr_blocks == 1, "Cantidad de bloques es 1");
	assert_ok(stats.total_blocks == 1, "Cantidad total de bloques es 1");
	assert_ok(stats.curr_regions == 2, "Cantidad de regiones es 2");
	assert_ok(stats.splitted_amnt == 1, "Se realizaron 1 splits");
	assert_ok(stats.mapped_amnt == BLOCK_SM, "Tamanio del heap es 16KiB" );
	free(ptr);
}

void
test_malloc_gt_64_sm_block(){
	YELLOW("LLAMADA A MALLOC CON 64 < x < 16KiB")
	char* ptr = malloc(150);
	assert_ok(ptr, "Malloc con 150 no es null");
	assert_ok(stats.malloc_calls == 1, "Llamadas a malloc es 1");
	assert_ok(stats.requested_amnt == 150, "Cantidad pedida es 150");
	assert_ok(stats.given_amnt == ALIGN4(150), "Cantidad recibida es 152");
	assert_ok(stats.curr_blocks == 1, "Cantidad de bloques es 1");
	assert_ok(stats.total_blocks == 1, "Cantidad total de bloques es 1");
	assert_ok(stats.curr_regions == 2, "Cantidad de regiones es 2");
	assert_ok(stats.splitted_amnt == 1, "Se realizaron 1 splits");
	assert_ok(stats.mapped_amnt == BLOCK_SM, "Tamanio del heap es 16KiB" );
	free(ptr);
}

void
test_malloc_close_to_16kb(){
	YELLOW("LLAMADA A MALLOC CON CERCA 16KiB")
	char* ptr = malloc(BLOCK_SM-40);
	assert_ok(ptr, "Malloc con ~16KiB no es null");
	assert_ok(stats.malloc_calls == 1, "Llamadas a malloc es 1");
	assert_ok(stats.requested_amnt == BLOCK_SM-40, "Cantidad pedida es correcta");
	assert_ok(stats.given_amnt == ALIGN4(BLOCK_SM-40), "Cantidad recibida es correcta");
	assert_ok(stats.curr_blocks == 1, "Cantidad de bloques es 1");
	assert_ok(stats.total_blocks == 1, "Cantidad total de bloques es 1");
	assert_ok(stats.curr_regions == 1, "Cantidad de regiones es 1");
	assert_ok(stats.splitted_amnt == 0, "Se realizaron 0 splits");
	assert_ok(stats.mapped_amnt == BLOCK_SM, "Tamanio del heap es 16KiB" );
	free(ptr);
}

void
test_malloc_md_block(){
	YELLOW("LLAMADA A MALLOC CON 16KiB < x < 1MiB")
	char* ptr = malloc(BLOCK_SM + 150);
	assert_ok(ptr, "Malloc con x no es null");
	assert_ok(stats.malloc_calls == 1, "Llamadas a malloc es 1");
	assert_ok(stats.requested_amnt == BLOCK_SM + 150, "Cantidad pedida es x");
	assert_ok(stats.given_amnt == ALIGN4(BLOCK_SM + 150), "Cantidad recibida es correcta");
	assert_ok(stats.curr_blocks == 1, "Cantidad de bloques es 1");
	assert_ok(stats.total_blocks == 1, "Cantidad total de bloques es 1");
	assert_ok(stats.curr_regions == 2, "Cantidad de regiones es 2");
	assert_ok(stats.splitted_amnt == 1, "Se realizaron 1 splits");
	assert_ok(stats.mapped_amnt == BLOCK_MD, "Tamanio del heap es 16KiB" );
	free(ptr);
}

void
test_malloc_lg_block(){
	YELLOW("LLAMADA A MALLOC CON 1MiB < x < 32MiB")
	char* ptr = malloc(BLOCK_MD + 150);
	assert_ok(ptr, "Malloc con x no es null");
	assert_ok(stats.malloc_calls == 1, "Llamadas a malloc es 1");
	assert_ok(stats.requested_amnt == BLOCK_MD + 150, "Cantidad pedida es x");
	assert_ok(stats.given_amnt == ALIGN4(BLOCK_MD + 150), "Cantidad recibida es correcta");
	assert_ok(stats.curr_blocks == 1, "Cantidad de bloques es 1");
	assert_ok(stats.total_blocks == 1, "Cantidad total de bloques es 1");
	assert_ok(stats.curr_regions == 2, "Cantidad de regiones es 2");
	assert_ok(stats.splitted_amnt == 1, "Se realizaron 1 splits");
	assert_ok(stats.mapped_amnt == BLOCK_LG, "Tamanio del heap es 16KiB" );
	free(ptr);
}

void 
test_multiple_blocks()
{
	YELLOW("LLAMADA MULTIPLES A MALLOC")
	char* ptr1  = malloc(64);
	char* ptr11 = malloc(64);
	char* ptr12 = malloc(64);
	assert_ok(stats.malloc_calls == 3, "Llamadas a malloc es 3");
	assert_ok(stats.requested_amnt == 64*3, "Cantidad pedida es correcta");
	assert_ok(stats.given_amnt == 64*3, "Cantidad recibida es correcta");
	assert_ok(stats.curr_blocks == 1, "Cantidad de bloques es 1");
	assert_ok(stats.total_blocks == 1, "Cantidad total de bloques es 1");
	assert_ok(stats.curr_regions == 4, "Cantidad de regiones es 4");
	assert_ok(stats.splitted_amnt == 3, "Se realizaron 1 splits");
	assert_ok(stats.mapped_amnt == BLOCK_SM, "Tamanio del heap es 16KiB" );

	char* ptr2  = malloc(BLOCK_SM + 150);
	char* ptr3  = malloc(BLOCK_MD + 150);

	assert_ok(stats.malloc_calls == 5, "Llamadas a malloc es 5");
	assert_ok(stats.requested_amnt == 64*3+BLOCK_SM + 150+BLOCK_MD + 150, "Cantidad pedida es correcta");
	assert_ok(stats.given_amnt == 64*3 + ALIGN4(BLOCK_SM + 150) + ALIGN4(BLOCK_MD + 150), "Cantidad recibida es correcta");
	assert_ok(stats.curr_blocks == 3, "Cantidad de bloques es 3");
	assert_ok(stats.total_blocks == 3, "Cantidad total de bloques es 3");
	assert_ok(stats.curr_regions == 8, "Cantidad de regiones es 8");
	assert_ok(stats.splitted_amnt == 5, "Se realizaron 1 splits");
	assert_ok(stats.mapped_amnt == BLOCK_LG + BLOCK_MD + BLOCK_SM, "Tamanio del heap es correcto" );
	free(ptr1);
	free(ptr11);
	free(ptr12);
	free(ptr2);
	free(ptr3);
}

void
test_malloc()
{
	RUN_TEST(test_malloc_with_zero)
	CLEAN_STATS

	RUN_TEST(test_malloc_lt_64)
	CLEAN_STATS

	RUN_TEST(test_malloc_gt_64_sm_block)
	CLEAN_STATS

	RUN_TEST(test_malloc_close_to_16kb)
	CLEAN_STATS

	RUN_TEST(test_malloc_md_block)
	CLEAN_STATS

	RUN_TEST(test_malloc_lg_block)
	CLEAN_STATS

	RUN_TEST(test_multiple_blocks)
	CLEAN_STATS
}

void
test_calloc_contains_zero()
{
	YELLOW("LLAMADA A CALLOC CONTAINS ZERO")
	char* ptr = calloc(1,150);

	bool ok = true;
	for(int i = 0; i < 150; i++){
		if(ptr[i] != 0)
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

void
test_realloc()
{
	YELLOW("TO BE IMPLEMENTED")
}

void 
test_return_mmy_to_OS()
{
	YELLOW("LIBERO BLOQUE Y SE DEVUELVE AL SISTEMA")
	char* ptr = malloc(150);
	printfmt("%d", ptr);
	free(ptr);

	assert_ok(stats.free_calls == 1, "Cantidad de free calls es correcta");
	assert_ok(stats.freed_amnt == ALIGN4(150), "Cantidad de memoria liberada es correcta");
	assert_ok(stats.curr_blocks == 0, "No hay bloques actuales");
	assert_ok(stats.coalesced_amnt == 1, "Se realizo un coalesce");
	assert_ok(stats.total_blocks == 1, "Se pidio un bloque antes de liberarlo");
	assert_ok(stats.returned_blocks == 1, "Se devolvio un bloque");
}

void
test_free()
{
	RUN_TEST(test_return_mmy_to_OS)
	CLEAN_STATS
}

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
