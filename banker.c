
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


/*
 * Inicializa as estruturas de alocacao e necessidade.
 */
int initialize_banker(Banker *banker) {
    banker->allocation = allocate_matrix(
        banker->number_of_customers,
        banker->number_of_resources
    );

    banker->need = allocate_matrix(
        banker->number_of_customers,
        banker->number_of_resources
    );

    if (banker->allocation == NULL || banker->need == NULL) {
        fprintf(stderr, "Erro: falha ao alocar matrizes do Banker.\n");
        return 0;
    }

    for (int i = 0; i < banker->number_of_customers; i++) {
        for (int j = 0; j < banker->number_of_resources; j++) {
            banker->need[i][j] = banker->maximum[i][j];
        }
    }

    return 1;
}


/*
 * Exibe o estado atual do algoritmo Banker.
 */
void print_state(Banker *banker) {
    printf("\nMAXIMUM\n");

    for (int i = 0; i < banker->number_of_customers; i++) {
        for (int j = 0; j < banker->number_of_resources; j++) {
            printf("%d ", banker->maximum[i][j]);
        }
        printf("\n");
    }

    printf("\nALLOCATION\n");

    for (int i = 0; i < banker->number_of_customers; i++) {
        for (int j = 0; j < banker->number_of_resources; j++) {
            printf("%d ", banker->allocation[i][j]);
        }
        printf("\n");
    }

    printf("\nNEED\n");

    for (int i = 0; i < banker->number_of_customers; i++) {
        for (int j = 0; j < banker->number_of_resources; j++) {
            printf("%d ", banker->need[i][j]);
        }
        printf("\n");
    }

    printf("\nAVAILABLE\n");

    for (int j = 0; j < banker->number_of_resources; j++) {
        printf("%d ", banker->available[j]);
    }

    printf("\n");
}


/*
 * Verifica se o estado atual do sistema e seguro.
 * Retorna 1 se for seguro, 0 se for inseguro e -1 em caso de erro.
 */
int is_safe_state(Banker *banker) {
    int *work = malloc(
        banker->number_of_resources * sizeof(int)
    );

    int *finish = calloc(
        banker->number_of_customers, sizeof(int)
    );

    if (work == NULL || finish == NULL) {
        free(work);
        free(finish);
        fprintf(stderr, "Erro: falha ao verificar estado seguro.\n");
        return -1;
    }

    for (int j = 0; j < banker->number_of_resources; j++) {
        work[j] = banker->available[j];
    }

    int finished_count = 0;
    int progress = 1;

    while (finished_count < banker->number_of_customers && progress) {
        progress = 0;

        for (int i = 0; i < banker->number_of_customers; i++) {
            if (finish[i]) {
                continue;
            }

            int can_finish = 1;

            for (int j = 0; j < banker->number_of_resources; j++) {
                if (banker->need[i][j] > work[j]) {
                    can_finish = 0;
                    break;
                }
            }

            if (can_finish) {
                for (int j = 0; j < banker->number_of_resources; j++) {
                    work[j] += banker->allocation[i][j];
                }

                finish[i] = 1;
                finished_count++;
                progress = 1;
            }
        }
    }

    free(work);
    free(finish);

    return finished_count == banker->number_of_customers;
}


/*
 * Processa uma solicitacao de recursos de um cliente.
 * Retorna 1 se aceita, 0 se negada e -1 em caso de erro.
 */
int request_resources(Banker *banker, int customer,
                      int request[]) {
    if (customer < 0 || customer >= banker->number_of_customers) {
        fprintf(stderr, "Erro: cliente invalido.\n");
        return 0;
    }

    /* Verifica se o pedido nao ultrapassa a necessidade. */
    for (int j = 0; j < banker->number_of_resources; j++) {
        if (request[j] < 0 || request[j] > banker->need[customer][j]) {
            printf("Solicitacao negada: pedido maior que a necessidade.\n");
            return 0;
        }
    }

    /* Verifica se existem recursos disponiveis. */
    for (int j = 0; j < banker->number_of_resources; j++) {
        if (request[j] > banker->available[j]) {
            printf("Solicitacao negada: recursos indisponiveis.\n");
            return 0;
        }
    }

    /* Simula a alocacao dos recursos. */
    for (int j = 0; j < banker->number_of_resources; j++) {
        banker->available[j] -= request[j];
        banker->allocation[customer][j] += request[j];
        banker->need[customer][j] -= request[j];
    }

    /* Verifica se o novo estado continua seguro. */
    int safe = is_safe_state(banker);

    if (safe == -1) {
        /* Desfaz a simulacao em caso de erro. */
        for (int j = 0; j < banker->number_of_resources; j++) {
            banker->available[j] += request[j];
            banker->allocation[customer][j] -= request[j];
            banker->need[customer][j] += request[j];
        }

        return -1;
    }

    if (!safe) {
        /* Desfaz a solicitacao se o estado ficar inseguro. */
        for (int j = 0; j < banker->number_of_resources; j++) {
            banker->available[j] += request[j];
            banker->allocation[customer][j] -= request[j];
            banker->need[customer][j] += request[j];
        }

        printf("Solicitacao negada: estado inseguro.\n");
        return 0;
    }

    printf("Solicitacao aprovada.\n");
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

    if (!initialize_banker(&banker)) {
        free_banker(&banker);
        return EXIT_FAILURE;
    }

    print_state(&banker);

    int safe = is_safe_state(&banker);

    if (safe == 1) {
        printf("\nEstado seguro.\n");
    } else if (safe == 0) {
        printf("\nEstado inseguro.\n");
    } else {
        free_banker(&banker);
        return EXIT_FAILURE;
    }

    int request[] = {0, 1, 0};

    request_resources(&banker, 0, request);

    print_state(&banker);

    printf("Recursos: %d\n", banker.number_of_resources);
    printf("Clientes: %d\n", banker.number_of_customers);

    free_banker(&banker);

    return EXIT_SUCCESS;
}
