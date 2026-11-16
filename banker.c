
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef struct {
    int **maximum;
    int **allocation;
    int **need;
    int *available;

    int number_of_customers;
    int number_of_resources;
} Banker;

/*
 * Libera a memoria alocada para uma matriz.
 */
void free_matrix(int **matrix, int rows) {
    if (matrix == NULL) {
        return;
    }

    for (int i = 0; i < rows; i++) {
        free(matrix[i]);
    }

    free(matrix);
}

/*
 * Libera toda a memoria utilizada pelo Banker.
 */
void free_banker(Banker *banker) {
    free_matrix(banker->maximum, banker->number_of_customers);
    free_matrix(banker->allocation, banker->number_of_customers);
    free_matrix(banker->need, banker->number_of_customers);

    free(banker->available);

    memset(banker, 0, sizeof(Banker));
}

/*
 * Aloca uma matriz de inteiros.
 */
int **allocate_matrix(int rows, int columns) {
    int **matrix = calloc((size_t) rows, sizeof(int *));

    if (matrix == NULL) {
        return NULL;
    }

    for (int i = 0; i < rows; i++) {
        matrix[i] = calloc((size_t) columns, sizeof(int));

        if (matrix[i] == NULL) {
            free_matrix(matrix, rows);
            return NULL;
        }
    }

    return matrix;
}

/*
 * Le e valida os argumentos de recursos.
 */
int read_resources(int argc, char *argv[], Banker *banker) {
    if (argc < 2) {
        fprintf(stderr, "Erro: informe a quantidade de recursos.\n");
        return 0;
    }

    banker->number_of_resources = argc - 1;

    banker->available = calloc(
        (size_t) banker->number_of_resources,
        sizeof(int)
    );

    if (banker->available == NULL) {
        fprintf(stderr, "Erro: falha ao alocar memoria.\n");
        return 0;
    }

    for (int i = 0; i < banker->number_of_resources; i++) {
        char *endptr;
        long value = strtol(argv[i + 1], &endptr, 10);

        if (argv[i + 1][0] == '\0' ||
            *endptr != '\0' ||
            value <= 0 ||
            value > INT_MAX) {

            fprintf(stderr,
                    "Erro: quantidade de recursos invalida: %s\n",
                    argv[i + 1]);

            return 0;
        }

        banker->available[i] = (int) value;
    }

    return 1;
}

/*
 * Le o arquivo customer.txt.
 * Cada linha representa um cliente.
 */
int read_customers(Banker *banker) {
    FILE *file = fopen("customer.txt", "r");

    if (file == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir customer.txt.\n");
        return 0;
    }

    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    int customers = 0;

    while ((length = getline(&line, &capacity, file)) != -1) {
        int values[banker->number_of_resources];

        char *cursor = line;

        for (int j = 0; j < banker->number_of_resources; j++) {
            while (*cursor == ' ' || *cursor == '\t') {
                cursor++;
            }

            char *endptr;
            long value = strtol(cursor, &endptr, 10);

            if (cursor == endptr ||
                value < 0 ||
                value > INT_MAX) {

                fprintf(stderr,
                        "Erro: valor invalido na linha %d de customer.txt.\n",
                        customers + 1);

                free(line);
                fclose(file);
                return 0;
            }

            values[j] = (int) value;
            cursor = endptr;

            while (*cursor == ' ' || *cursor == '\t') {
                cursor++;
            }

            if (j < banker->number_of_resources - 1) {
                if (*cursor != ',') {
                    fprintf(stderr,
                            "Erro: formato invalido na linha %d de customer.txt.\n",
                            customers + 1);

                    free(line);
                    fclose(file);
                    return 0;
                }

                cursor++;
            }
        }

        while (*cursor == ' ' || *cursor == '\t' ||
               *cursor == '\r' || *cursor == '\n') {
            cursor++;
        }

        if (*cursor != '\0') {
            fprintf(stderr,
                    "Erro: quantidade incorreta de recursos na linha %d.\n",
                    customers + 1);

            free(line);
            fclose(file);
            return 0;
        }

        int **new_matrix = realloc(
            banker->maximum,
            (size_t) (customers + 1) * sizeof(int *)
        );

        if (new_matrix == NULL) {
            fprintf(stderr, "Erro: falha ao alocar memoria.\n");

            free(line);
            fclose(file);
            return 0;
        }

        banker->maximum = new_matrix;

        banker->maximum[customers] = malloc(
            (size_t) banker->number_of_resources * sizeof(int)
        );

        if (banker->maximum[customers] == NULL) {
            fprintf(stderr, "Erro: falha ao alocar memoria.\n");

            free(line);
            fclose(file);
            return 0;
        }

        for (int j = 0; j < banker->number_of_resources; j++) {
            if (values[j] > banker->available[j]) {
                fprintf(stderr,
                        "Erro: demanda maxima do cliente %d excede os recursos do sistema.\n",
                        customers);

                free(line);
                fclose(file);
                return 0;
            }

            banker->maximum[customers][j] = values[j];
        }

        customers++;
    }

    free(line);

    if (ferror(file)) {
        fprintf(stderr, "Erro ao ler customer.txt.\n");
        fclose(file);
        return 0;
    }

    fclose(file);

    if (customers == 0) {
        fprintf(stderr, "Erro: customer.txt nao possui clientes.\n");
        return 0;
    }

    banker->number_of_customers = customers;

    return 1;
}

int main(int argc, char *argv[]) {
    Banker banker = {0};

    if (!read_resources(argc, argv, &banker)) {
        free_banker(&banker);
        return EXIT_FAILURE;
    }

    if (!read_customers(&banker)) {
        free_banker(&banker);
        return EXIT_FAILURE;
    }

    printf("Recursos: %d\n", banker.number_of_resources);
    printf("Clientes: %d\n", banker.number_of_customers);

    free_banker(&banker);

    return EXIT_SUCCESS;
}
