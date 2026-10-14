#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include "syntax_analyzer.h"

int main(void){
    printf("Database Project tldairy dnguye24\n");
    printf("=====================================================\n");
    char separated_statements[100][MAX_BUFFER]; //100 Statements Only
    int num_statements = 0;

    statement_separator(separated_statements, &num_statements);

    printf("Number of statements: %d\n", num_statements);
    for (int i = 0; i < num_statements; i++) {
        printf("\nStatement number: %d\n", i + 1);
        if (!statement_identifier(separated_statements[i])) {
            break;
        }
    }

    return 0;
}