# Algoritmo de Banker

Implementação do Algoritmo de Banker em linguagem C, desenvolvida como atividade acadêmica da disciplina de Sistemas Operacionais.

## Objetivo

Implementar o algoritmo de Banker para gerenciamento de recursos, verificando a segurança dos estados do sistema antes de conceder solicitações e evitando situações de deadlock.

## Funcionalidades

- Leitura da quantidade de recursos por argumentos de linha de comando.
- Leitura das demandas máximas dos clientes através de `customer.txt`.
- Processamento de solicitações e liberações através de `commands.txt`.
- Verificação de estados seguros e inseguros.
- Controle de alocação, necessidade e disponibilidade de recursos.
- Registro dos resultados da execução em `result.txt`.
- Tratamento de erros de entrada e arquivos.

## Tecnologias utilizadas

- Linguagem C
- GCC
- Makefile
- Git e GitHub
- Linux / WSL

## Estrutura do projeto

| Arquivo | Descrição |
|---|---|
| `banker.c` | Implementação do algoritmo |
| `Makefile` | Compilação e limpeza dos arquivos gerados |
| `customer.txt` | Demandas máximas dos clientes |
| `commands.txt` | Comandos de solicitação e liberação |
| `result.txt` | Resultados da execução |
| `evidencias.log` | Registro das evidências de execução |

## Compilação

Para compilar o programa, execute:

```bash
make
```

O comando deverá gerar o executável `banker`.

## Execução

Informe a quantidade de instâncias disponíveis de cada tipo de recurso como argumentos:

```bash
./banker 10 5 7
```

O programa utilizará os arquivos `customer.txt` e `commands.txt` para processar as operações.

## Autoria

Projeto acadêmico desenvolvido individualmente para a disciplina de Sistemas Operacionais.
