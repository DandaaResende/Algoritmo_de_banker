#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>

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

        errno = 0;
        long value = strtol(argv[i + 1], &endptr, 10);

        if (argv[i + 1][0] == '\0' ||
            *endptr != '\0' ||
            errno == ERANGE ||
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
        (void) length;

        int values[banker->number_of_resources];
        char *cursor = line;

        for (int j = 0; j < banker->number_of_resources; j++) {
            while (*cursor == ' ' || *cursor == '\t') {
                cursor++;
            }

            char *endptr;
            errno = 0;
            long value = strtol(cursor, &endptr, 10);

            if (cursor == endptr ||
                errno == ERANGE ||
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

        /* Confere se a demanda maxima cabe nos recursos do sistema. */
        for (int j = 0; j < banker->number_of_resources; j++) {
            if (values[j] > banker->available[j]) {
                fprintf(stderr,
                        "Erro: demanda maxima do cliente %d excede os recursos do sistema.\n",
                        customers);

                free(line);
                fclose(file);
                return 0;
            }
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
            banker->maximum[customers][j] = values[j];
        }

        customers++;
        banker->number_of_customers = customers;
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

void print_state(Banker *banker, FILE *output) {
    fprintf(output, "\nMAXIMUM\n");

    for (int i = 0; i < banker->number_of_customers; i++) {
        for (int j = 0; j < banker->number_of_resources; j++) {
            fprintf(output, "%d ", banker->maximum[i][j]);
        }
        fprintf(output, "\n");
    }

    fprintf(output, "\nALLOCATION\n");

    for (int i = 0; i < banker->number_of_customers; i++) {
        for (int j = 0; j < banker->number_of_resources; j++) {
            fprintf(output, "%d ", banker->allocation[i][j]);
        }
        fprintf(output, "\n");
    }

    fprintf(output, "\nNEED\n");

    for (int i = 0; i < banker->number_of_customers; i++) {
        for (int j = 0; j < banker->number_of_resources; j++) {
            fprintf(output, "%d ", banker->need[i][j]);
        }
        fprintf(output, "\n");
    }

    fprintf(output, "\nAVAILABLE\n");

    for (int j = 0; j < banker->number_of_resources; j++) {
        fprintf(output, "%d ", banker->available[j]);
    }

    fprintf(output, "\n");
}


/*
 * Verifica se o estado atual do sistema e seguro.
 * Retorna 1 se for seguro, 0 se for inseguro e -1 em caso de erro.
 */
int is_safe_state(Banker *banker) {
    int *work = malloc(
        (size_t) banker->number_of_resources * sizeof(int)
    );

    int *finish = calloc(
        (size_t) banker->number_of_customers, sizeof(int)
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
                      int request[], FILE *output) {
    if (customer < 0 || customer >= banker->number_of_customers) {
        fprintf(stderr, "Erro: cliente invalido.\n");
        return 0;
    }

    for (int j = 0; j < banker->number_of_resources; j++) {
        if (request[j] < 0 || request[j] > banker->need[customer][j]) {
            fprintf(output,
                    "Solicitacao negada: pedido maior que a necessidade.\n");
            return 0;
        }
    }

    for (int j = 0; j < banker->number_of_resources; j++) {
        if (request[j] > banker->available[j]) {
            fprintf(output,
                    "Solicitacao negada: recursos indisponiveis.\n");
            return 0;
        }
    }

    /* Simula a alocacao dos recursos. */
    for (int j = 0; j < banker->number_of_resources; j++) {
        banker->available[j] -= request[j];
        banker->allocation[customer][j] += request[j];
        banker->need[customer][j] -= request[j];
    }

    int safe = is_safe_state(banker);

    if (safe == -1 || !safe) {
        /* Desfaz a simulacao se houver erro ou estado inseguro. */
        for (int j = 0; j < banker->number_of_resources; j++) {
            banker->available[j] += request[j];
            banker->allocation[customer][j] -= request[j];
            banker->need[customer][j] += request[j];
        }

        if (safe == -1) {
            return -1;
        }

        fprintf(output, "Solicitacao negada: estado inseguro.\n");
        return 0;
    }

    fprintf(output, "Solicitacao aprovada.\n");
    return 1;
}

/*
 * Processa a liberacao de recursos de um cliente.
 * Retorna 1 se aceita e 0 se negada.
 */


int release_resources(Banker *banker, int customer,
                      int release[], FILE *output) {
    if (customer < 0 || customer >= banker->number_of_customers) {
        fprintf(stderr, "Erro: cliente invalido.\n");
        return 0;
    }

    for (int j = 0; j < banker->number_of_resources; j++) {
        if (release[j] < 0 ||
            release[j] > banker->allocation[customer][j]) {
            fprintf(output,
                    "Liberacao negada: quantidade invalida.\n");
            return 0;
        }
    }

    for (int j = 0; j < banker->number_of_resources; j++) {
        banker->available[j] += release[j];
        banker->allocation[customer][j] -= release[j];
        banker->need[customer][j] += release[j];
    }

    fprintf(output, "Liberacao aprovada.\n");
    return 1;
}


/*
 * Converte um texto em inteiro, verificando se o valor e valido.
 */
int parse_integer(const char *text, int *value) {
    char *end;
    errno = 0;

    long number = strtol(text, &end, 10);

    if (text == end ||
        *end != '\0' ||
        errno == ERANGE ||
        number < INT_MIN ||
        number > INT_MAX) {
        return 0;
    }

    *value = (int) number;
    return 1;
}

/*
 * Le os comandos do arquivo commands.txt e executa cada operacao.
 */

int process_commands(Banker *banker) {
    FILE *file = fopen("commands.txt", "r");

    if (file == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir commands.txt.\n");
        return 0;
    }

    FILE *output = fopen("result.txt", "w");

    if (output == NULL) {
        fprintf(stderr, "Erro: nao foi possivel criar result.txt.\n");
        fclose(file);
        return 0;
    }

    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    int line_number = 0;

    int *values = malloc(
        ((size_t) banker->number_of_resources + 1) * sizeof(int)
    );

    if (values == NULL) {
        fprintf(stderr, "Erro: falha ao alocar memoria.\n");
        fclose(file);
        fclose(output);
        return 0;
    }

    while ((length = getline(&line, &capacity, file)) != -1) {
        (void) length;
        line_number++;

        char *saveptr;
        char *token = strtok_r(line, " \t\r\n", &saveptr);

        /* Ignora linhas vazias. */
        if (token == NULL) {
            continue;
        }

        /* Comando para exibir o estado atual. */
        if (strcmp(token, "*") == 0) {
            if (strtok_r(NULL, " \t\r\n", &saveptr) != NULL) {
                fprintf(stderr,
                        "Erro na linha %d: comando * invalido.\n",
                        line_number);
                goto error;
            }

            print_state(banker, output);
            continue;
        }

        int is_request = strcmp(token, "RQ") == 0;
        int is_release = strcmp(token, "RL") == 0;

        if (!is_request && !is_release) {
            fprintf(stderr,
                    "Erro na linha %d: comando desconhecido.\n",
                    line_number);
            goto error;
        }

        /* Le o cliente e as quantidades de recursos. */
        for (int i = 0; i <= banker->number_of_resources; i++) {
            token = strtok_r(NULL, " \t\r\n", &saveptr);

            if (token == NULL || !parse_integer(token, &values[i])) {
                fprintf(stderr,
                        "Erro na linha %d: parametros invalidos.\n",
                        line_number);
                goto error;
            }
        }

        /* Rejeita parametros extras. */
        if (strtok_r(NULL, " \t\r\n", &saveptr) != NULL) {
            fprintf(stderr,
                    "Erro na linha %d: parametros extras.\n",
                    line_number);
            goto error;
        }

        int customer = values[0];
        int *resources = &values[1];

        if (is_request) {
            int result = request_resources(
                banker, customer, resources, output
            );

            if (result == -1) {
                goto error;
            }
        } else {
            release_resources(
                banker, customer, resources, output
            );
        }
    }

    if (ferror(file)) {
        fprintf(stderr, "Erro ao ler commands.txt.\n");
        goto error;
    }

    free(values);
    free(line);
    fclose(file);

    if (fclose(output) != 0) {
        fprintf(stderr, "Erro ao finalizar result.txt.\n");
        return 0;
    }

    return 1;

error:
    free(values);
    free(line);
    fclose(file);
    fclose(output);

    return 0;
}


/*
 * Funcao principal.
 */
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

    if (!process_commands(&banker)) {
        free_banker(&banker);
        return EXIT_FAILURE;
    }

    free_banker(&banker);

    return EXIT_SUCCESS;
}