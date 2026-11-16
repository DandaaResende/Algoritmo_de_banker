
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/*
 * Estruturas e variaveis do Algoritmo de Banker.
 */

typedef struct {
    int **maximum;
    int **allocation;
    int **need;
    int *available;

    int number_of_customers;
    int number_of_resources;
} Banker;

int main(int argc, char *argv[]) {
    Banker banker = {0};

    (void) argc;
    (void) argv;

    printf("Algoritmo de Banker\n");

    return 0;
}
