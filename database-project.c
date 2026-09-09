#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

const size_t MAX_BUFFER = 256;


void tokenize(char *query, char tokens[][32], int *num_tokens) {
    char *token = strtok(query, " ");
    *num_tokens = 0;

    while (token != NULL) {
        strcpy(tokens[*num_tokens], token);
        (*num_tokens)++;
        token = strtok(NULL, " ,();");
    }

    for (int i = 0; i < *num_tokens; i++) {
        printf("Token %d: %s\n", i + 1, tokens[i]);
    }
}

void statement_separator(char separated_statements[][MAX_BUFFER], int* num_statements) {
    FILE *fp = stdin;

    if (isatty(fileno(stdin))) {
        fp = fopen("test.txt", "r");
    }

    if (fp == NULL) {
        printf("Error opening file\n");
        return;
    }

    char buffer[MAX_BUFFER];
    int index = 0;
    int ch;
    char delimiter = ';';

    *num_statements = 0;
    memset(buffer, 0, sizeof(buffer));

    while ((ch = fgetc(fp)) != EOF) {
        if (ch == delimiter) {
            buffer[index] = '\0';

            if (index > 0) {
                strcpy(separated_statements[*num_statements], buffer);
                (*num_statements)++;
            }

            index = 0;
            memset(buffer, 0, sizeof(buffer));
            continue;
        }

        if (index < (int)MAX_BUFFER - 1) {
            if (ch == '\n' || ch == '\r') {
                buffer[index++] = ' ';
            } else {
                buffer[index++] = (char)ch;
            }
        }
    }

    if (index > 0) {
        buffer[index] = '\0';
        strcpy(separated_statements[*num_statements], buffer);
        (*num_statements)++;
    }

    fclose(fp);

}

// void tokenizer (char *query) {


//     char delimChar = '-'; // The delimiter character
    
//     // strtok expects a string of delimiters, so we pass &c or a string literal like "-"
//     char *token = strtok(query, &delimChar);

//     while (token != NULL) {
//         printf("%s\n", token);
//         token = strtok(NULL, &delimChar);
//     }
//     return 0;
// }

void identify_tokens (char *statement){
    char *TYPES[] = {"KEYWORD", "IDENTIFIER", "OPERATOR", "LITERAL", "SEPARATOR"};

}

void create_parser(char *statement) {
    printf("Creating statement:\n \t\t%s\n", statement);
    
}

void statement_identifier(char *statement) {
    printf("Identifying statement:\n \t\t%s\n", statement);

    char *command = strtok(statement, " ");
    if (command == NULL) {
        printf("Empty statement\n");
        return;
    }
    if (strcmp(strlwr(command), "create") == 0){
        printf("\t\tCREATE statement identified\n");
    }else if (strcmp(strlwr(command), "use") == 0){
        printf("\t\tUSE statement identified\n");
    }else if (strcmp(strlwr(command), "describe") == 0 || strcmp(strlwr(command), "desc") == 0){
        printf("\t\tDESCRIBE statement identified\n");
    }else if (strcmp(strlwr(command), "select") == 0){
        printf("\t\tSELECT statement identified\n");
    }else if (strcmp(strlwr(command), "let") == 0){
        printf("\t\tLET statement identified\n");
    }else if (strcmp(strlwr(command), "insert") == 0){
        printf("\t\tINSERT statement identified\n");
    }else if (strcmp(strlwr(command), "update") == 0){
        printf("\t\tUPDATE statement identified\n");
    }else if (strcmp(strlwr(command), "delete") == 0){
        printf("\t\tDELETE statement identified\n");
    }else if (strcmp(strlwr(command), "rename") == 0){
        printf("\t\tRENAME statement identified\n");
    }else if (strcmp(strlwr(command), "input") == 0){
        printf("\t\tINPUT statement identified\n");
    }else if (strcmp(strlwr(command), "exit") == 0){
        printf("\t\tEXIT statement identified\n");
    }else {
        printf("Unknown statement type: %s\n", command);
    }
}

int main(void)
{
    printf("Database project starter\n");
    char separated_statements[100][MAX_BUFFER]; //100 Statements Only
    int num_statements = 0;

    statement_separator(separated_statements, &num_statements);

    printf("Number of statements: %d\n", num_statements);
    for (int i = 0; i < num_statements; i++) {
        printf("%d", i + 1);
        statement_identifier(separated_statements[i]);
    }

    return 0;
}