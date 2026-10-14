#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include "globals.h"

void tokenize(char *query, char tokens[][32], int *num_tokens) {
    char *token = strtok(query, " ");
    *num_tokens = 0;

    while (token != NULL) {
        strcpy(tokens[*num_tokens], token);
        (*num_tokens)++;
        token = strtok(NULL, " ,();");
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

// Define supported token types
typedef enum {
    TOKEN_CREATE,
    TOKEN_USE,
    TOKEN_DESCRIBE,
    TOKEN_SELECT,
    TOKEN_LET,
    TOKEN_INSERT,
    TOKEN_UPDATE,
    TOKEN_DELETE,
    TOKEN_RENAME,
    TOKEN_INPUT,
    TOKEN_EXIT,

    TOKEN_FROM,
    TOKEN_VALUES,
    TOKEN_WHERE,
    TOKEN_AND,
    TOKEN_OR,

    //misc
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_COMMA,
    TOKEN_ASTERISK,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_GREATER,
    TOKEN_LESS,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS_EQUAL,
    TOKEN_FILENAME,
    TOKEN_EOF,
    TOKEN_ERROR,

    //Tokens for column data types
    TOKEN_TEXT,
    TOKEN_INTEGER,
    TOKEN_FLOAT,

    // Tokens for LET
    TOKEN_KEY,
} TokenType;

// void print_token_type(TokenType type) {
//     switch (type) {
//         case TOKEN_CREATE: printf("TOKEN_CREATE"); break;
//         case TOKEN_USE: printf("TOKEN_USE"); break;
//         case TOKEN_DESCRIBE: printf("TOKEN_DESCRIBE"); break;
//         case TOKEN_SELECT: printf("TOKEN_SELECT"); break;
//         case TOKEN_LET: printf("TOKEN_LET"); break;
//         case TOKEN_INSERT: printf("TOKEN_INSERT"); break;
//         case TOKEN_UPDATE: printf("TOKEN_UPDATE"); break;
//         case TOKEN_DELETE: printf("TOKEN_DELETE"); break;
//         case TOKEN_RENAME: printf("TOKEN_RENAME"); break;
//         case TOKEN_INPUT: printf("TOKEN_INPUT"); break;
//         case TOKEN_EXIT: printf("TOKEN_EXIT"); break;
//         case TOKEN_FROM: printf("TOKEN_FROM"); break;
//         case TOKEN_VALUES: printf("TOKEN_VALUES"); break;
//         case TOKEN_WHERE: printf("TOKEN_WHERE"); break;
//         case TOKEN_AND: printf("TOKEN_AND"); break;
//         case TOKEN_OR: printf("TOKEN_OR"); break;
//         case TOKEN_IDENTIFIER: printf("TOKEN_IDENTIFIER"); break;
//         case TOKEN_NUMBER: printf("TOKEN_NUMBER"); break;
//         case TOKEN_STRING: printf("TOKEN_STRING"); break;
//         case TOKEN_COMMA: printf("TOKEN_COMMA"); break;
//         case TOKEN_ASTERISK: printf("TOKEN_ASTERISK"); break;
//         case TOKEN_LPAREN: printf("TOKEN_LPAREN"); break;
//         case TOKEN_RPAREN: printf("TOKEN_RPAREN"); break;
//         case TOKEN_EQUAL: printf("TOKEN_EQUAL"); break;
//         case TOKEN_NOT_EQUAL: printf("TOKEN_NOT_EQUAL"); break;
//         case TOKEN_GREATER: printf("TOKEN_GREATER"); break;
//         case TOKEN_LESS: printf("TOKEN_LESS"); break;
//         case TOKEN_GREATER_EQUAL: printf("TOKEN_GREATER_EQUAL"); break;
//         case TOKEN_LESS_EQUAL: printf("TOKEN_LESS_EQUAL"); break;
//         case TOKEN_EOF: printf("TOKEN_EOF"); break;
//         case TOKEN_ERROR: printf("TOKEN_ERROR"); break;
//     }
// }
// Token structure
typedef struct {
    TokenType type;
    char text[64];
} Token;

// Lexer/Parser State tracking
typedef struct {
    const char* source;
    size_t cursor;
    Token current_token;
} ParserState;

// Simple Lexer: Extract next token from source
Token next_token(ParserState* state) {
    Token token;
    token.text[0] = '\0';

    // Skip whitespace
    while (state->source[state->cursor] != '\0' && isspace(state->source[state->cursor])) {
        state->cursor++;
    }

    char current_char = state->source[state->cursor];

    if (current_char == '\0') {
        token.type = TOKEN_EOF;
        strcpy(token.text, "EOF");
        return token;
    }

    // Single character tokens
    if (current_char == ',') {
        token.type = TOKEN_COMMA;
        strcpy(token.text, ",");
        state->cursor++;
        return token;
    }
    if (current_char == '*') {
        token.type = TOKEN_ASTERISK;
        strcpy(token.text, "*");
        state->cursor++;
        return token;
    }
    if (current_char == '(') {
        token.type = TOKEN_LPAREN;
        strcpy(token.text, "(");
        state->cursor++;
        return token;
    }
    if (current_char == ')') {
        token.type = TOKEN_RPAREN;
        strcpy(token.text, ")");
        state->cursor++;
        return token;
    }

    if (current_char == '=') {
        token.type = TOKEN_EQUAL;
        strcpy(token.text, "=");
        state->cursor++;
        return token;
    }
    if (current_char == '>') {
        token.type = TOKEN_GREATER;
        strcpy(token.text, ">");
        state->cursor++;
        if (state->source[state->cursor] == '=') {
            token.type = TOKEN_GREATER_EQUAL;
            strcpy(token.text, ">=");
            state->cursor++;
        }
        return token;
    }
    if (current_char == '<') {
        token.type = TOKEN_LESS;
        strcpy(token.text, "<");
        state->cursor++;
        if (state->source[state->cursor] == '=') {
            token.type = TOKEN_LESS_EQUAL;
            strcpy(token.text, "<=");
            state->cursor++;
        }
        return token;
    }
    if (current_char == '!') {
        if (state->source[state->cursor + 1] == '=') {
            token.type = TOKEN_NOT_EQUAL;
            strcpy(token.text, "!=");
            state->cursor += 2;
            return token;
        }
        token.type = TOKEN_ERROR;
        token.text[0] = current_char;
        token.text[1] = '\0';
        state->cursor++;
        return token;
    }

    if (current_char == '"') {
        size_t len = 0;
        state->cursor++;
        while (state->source[state->cursor] != '\0' && state->source[state->cursor] != '"') {
            if (len < 63) token.text[len++] = state->source[state->cursor];
            state->cursor++;
        }
        token.text[len] = '\0';
        if (state->source[state->cursor] != '"') {
            token.type = TOKEN_ERROR;
            return token;
        }
        state->cursor++;
        token.type = TOKEN_STRING;
        return token;
    }

    // Numbers
    if (isdigit((unsigned char)current_char) ||
        (current_char == '-' && isdigit((unsigned char)state->source[state->cursor + 1]))) {
        size_t len = 0;
        if (current_char == '-') {
            token.text[len++] = current_char;
            state->cursor++;
        }
        while (isdigit(state->source[state->cursor])) {
            if (len < 63) token.text[len++] = state->source[state->cursor];
            state->cursor++;
        }
        if (state->source[state->cursor] == '.') {
            if (len < 63) token.text[len++] = state->source[state->cursor];
            state->cursor++;
            if (!isdigit((unsigned char)state->source[state->cursor])) {
                token.text[len] = '\0';
                token.type = TOKEN_ERROR;
                return token;
            }
            while (isdigit(state->source[state->cursor])) {
                if (len < 63) token.text[len++] = state->source[state->cursor];
                state->cursor++;
            }
        }
        token.text[len] = '\0';
        token.type = TOKEN_NUMBER;
        return token;
    }

    // Identifiers and Keywords
    if (isalpha(current_char) || current_char == '_') {
        size_t len = 0;
        while (isalnum(state->source[state->cursor]) || state->source[state->cursor] == '_') {
            if (len < 63) token.text[len++] = state->source[state->cursor];
            state->cursor++;
        }

        if (state->source[state->cursor] == '.' ||
            state->source[state->cursor] == '/' ||
            state->source[state->cursor] == '\\' ||
            state->source[state->cursor] == ':' ||
            state->source[state->cursor] == '-') {
            while (state->source[state->cursor] != '\0' &&
                   !isspace((unsigned char)state->source[state->cursor]) &&
                   state->source[state->cursor] != ',' &&
                   state->source[state->cursor] != '(' &&
                   state->source[state->cursor] != ')') {
                if (len < 63) token.text[len++] = state->source[state->cursor];
                state->cursor++;
            }
            token.text[len] = '\0';
            token.type = TOKEN_FILENAME;
            return token;
        }

        token.text[len] = '\0';

        if (strcasecmp(token.text, "CREATE") == 0) token.type = TOKEN_CREATE;
        else if (strcasecmp(token.text, "USE") == 0) token.type = TOKEN_USE;
        else if (strcasecmp(token.text, "DESCRIBE") == 0 || strcasecmp(token.text, "DESC") == 0) token.type = TOKEN_DESCRIBE;
        else if (strcasecmp(token.text, "SELECT") == 0) token.type = TOKEN_SELECT;
        else if (strcasecmp(token.text, "LET") == 0) token.type = TOKEN_LET;
        else if (strcasecmp(token.text, "INSERT") == 0) token.type = TOKEN_INSERT;
        else if (strcasecmp(token.text, "UPDATE") == 0) token.type = TOKEN_UPDATE;
        else if (strcasecmp(token.text, "DELETE") == 0) token.type = TOKEN_DELETE;
        else if (strcasecmp(token.text, "RENAME") == 0) token.type = TOKEN_RENAME;
        else if (strcasecmp(token.text, "INPUT") == 0) token.type = TOKEN_INPUT;
        else if (strcasecmp(token.text, "EXIT") == 0) token.type = TOKEN_EXIT;
        else if (strcasecmp(token.text, "FROM") == 0) token.type = TOKEN_FROM;
        else if (strcasecmp(token.text, "VALUES") == 0) token.type = TOKEN_VALUES;
        else if (strcasecmp(token.text, "WHERE") == 0) token.type = TOKEN_WHERE;
        else if (strcasecmp(token.text, "AND") == 0) token.type = TOKEN_AND;
        else if (strcasecmp(token.text, "OR") == 0) token.type = TOKEN_OR;
        else if (strcasecmp(token.text, "TEXT") == 0) token.type = TOKEN_TEXT;
        else if (strcasecmp(token.text, "INTEGER") == 0) token.type = TOKEN_INTEGER;
        else if (strcasecmp(token.text, "FLOAT") == 0) token.type = TOKEN_FLOAT;
        else if (strcasecmp(token.text, "KEY") == 0) token.type = TOKEN_KEY;
        else token.type = TOKEN_IDENTIFIER;

        return token;
    }

    token.type = TOKEN_ERROR;
    token.text[0] = current_char;
    token.text[1] = '\0';
    state->cursor++;
    return token;
}

// Helper to advance the parser state
void advance(ParserState* state) {
    state->current_token = next_token(state);
}

// Recursive Descent Parser Functions
static int parse_select_item(ParserState* state, const char* required_key,
                             int* key_found, int* is_aggregate) {
    if (state->current_token.type == TOKEN_ASTERISK) {
        printf("[%s] ", state->current_token.text);
        if (required_key != NULL) {
            *key_found = 1;
        }
        advance(state);
        return 1;
    }

    if (state->current_token.type != TOKEN_IDENTIFIER) {
        return 0;
    }

    char func_name[64];
    strcpy(func_name, state->current_token.text);
    advance(state);

    if (state->current_token.type == TOKEN_LPAREN) {
        *is_aggregate = 1;
        if (strcasecmp(func_name, "max") != 0 &&
            strcasecmp(func_name, "min") != 0 &&
            strcasecmp(func_name, "average") != 0 &&
            strcasecmp(func_name, "count") != 0) {
            printf("\nSyntax Error: Unsupported aggregate function '%s'\n", func_name);
            return 0;
        }

        advance(state);
        if (strcasecmp(func_name, "count") == 0) {
            if (state->current_token.type != TOKEN_ASTERISK) {
                printf("\nSyntax Error: COUNT requires '*' as its argument\n");
                return 0;
            }
        } else if (state->current_token.type != TOKEN_IDENTIFIER) {
            printf("\nSyntax Error: Aggregate '%s' requires an attribute name\n", func_name);
            return 0;
        }

        printf("[%s(%s)] ", func_name, state->current_token.text);
        advance(state);
        if (state->current_token.type != TOKEN_RPAREN) {
            printf("\nSyntax Error: Missing ')' in aggregate call near '%s'\n",
                   state->current_token.text);
            return 0;
        }
        advance(state);
        return 1;
    }

    if (strcasecmp(func_name, "max") == 0 ||
        strcasecmp(func_name, "min") == 0 ||
        strcasecmp(func_name, "average") == 0 ||
        strcasecmp(func_name, "count") == 0) {
        printf("\nSyntax Error: Aggregate name '%s' must be called as a function\n", func_name);
        return 0;
    }

    if (required_key != NULL && strcasecmp(func_name, required_key) == 0) {
        *key_found = 1;
    }
    printf("[%s] ", func_name);
    return 1;
}

static int is_comparison_operator(TokenType type) {
    return type == TOKEN_EQUAL || type == TOKEN_NOT_EQUAL || type == TOKEN_GREATER ||
           type == TOKEN_LESS || type == TOKEN_GREATER_EQUAL || type == TOKEN_LESS_EQUAL;
}

static int parse_where(ParserState* state) {
    if (state->current_token.type != TOKEN_WHERE) {
        printf("Syntax Error: Expected WHERE near '%s'\n", state->current_token.text);
        return 0;
    }
    advance(state);

    while (1) {
        if (state->current_token.type != TOKEN_IDENTIFIER) {
            printf("Syntax Error: Expected attribute name in WHERE condition near '%s'\n",
                   state->current_token.text);
            return 0;
        }

        printf("  Condition: [%s] ", state->current_token.text);
        advance(state);

        if (!is_comparison_operator(state->current_token.type)) {
            printf("\nSyntax Error: Missing comparison operator near '%s'\n",
                   state->current_token.text);
            return 0;
        }

        printf("[op:%s] ", state->current_token.text);
        advance(state);

        if (state->current_token.type != TOKEN_IDENTIFIER &&
            state->current_token.type != TOKEN_NUMBER &&
            state->current_token.type != TOKEN_STRING) {
            printf("\nSyntax Error: Missing value in WHERE clause near '%s'\n",
                   state->current_token.text);
            return 0;
        }

        printf("[%s]\n", state->current_token.text);
        advance(state);

        if (state->current_token.type != TOKEN_AND && state->current_token.type != TOKEN_OR) {
            return 1;
        }

        printf("  Logic: [%s] ", state->current_token.text);
        advance(state);
    }
}

static int require_end_of_statement(ParserState* state) {
    if (state->current_token.type == TOKEN_EOF) {
        return 1;
    }
    printf("Syntax Error: Unexpected trailing token '%s'\n", state->current_token.text);
    return 0;
}

static int parse_select(ParserState* state, const char* required_key) {
    printf("Parsing SELECT statement:\n");

    if (state->current_token.type != TOKEN_SELECT) {
        printf("Syntax Error: Expected SELECT near '%s'\n", state->current_token.text);
        return 0;
    }
    advance(state); // Consume SELECT

    // Parse columns and aggregate expressions uniformly.
    printf("  Columns: ");
    int key_found = 0;
    int aggregate_seen = 0;
    int regular_item_seen = 0;
    while (1) {
        int item_is_aggregate = 0;
        if (!parse_select_item(state, required_key, &key_found, &item_is_aggregate)) {
            printf("\nSyntax Error: Expected column name, '*' or aggregate function near '%s'\n", state->current_token.text);
            return 0;
        }
        if (item_is_aggregate) {
            if (regular_item_seen || aggregate_seen) {
                printf("\nSyntax Error: Aggregate SELECT must contain exactly one aggregate expression\n");
                return 0;
            }
            aggregate_seen = 1;
        } else {
            if (aggregate_seen) {
                printf("\nSyntax Error: Aggregate SELECT must contain exactly one aggregate expression\n");
                return 0;
            }
            regular_item_seen = 1;
        }

        if (state->current_token.type == TOKEN_COMMA) {
            if (aggregate_seen) {
                printf("\nSyntax Error: Aggregate SELECT cannot contain a column list\n");
                return 0;
            }
            advance(state); // Consume comma and continue loop
        } else {
            break;
        }
    }
    printf("\n");

    // Parse FROM clause
    if (state->current_token.type == TOKEN_FROM) {
        advance(state); // Consume FROM
        printf("  From Tables: ");
        while (1) {
            if (state->current_token.type != TOKEN_IDENTIFIER) {
                printf("\nSyntax Error: Expected table name after FROM near '%s'\n", state->current_token.text);
                return 0;
            }

            printf("[%s] ", state->current_token.text);
            advance(state);

            if (state->current_token.type == TOKEN_COMMA) {
                advance(state); // Consume comma and continue with more table names
                continue;
            }
            break;
        }
        printf("\n");
    } else {
        printf("Syntax Error: Missing FROM clause near '%s'\n", state->current_token.text);
        return 0;
    }

    // Parse WHERE keyword
    if (state->current_token.type == TOKEN_WHERE) {
        if (!parse_where(state)) {
            return 0;
        }
    }

    if (!require_end_of_statement(state)) {
        return 0;
    }
    if (required_key != NULL && !key_found) {
        printf("Syntax Error: LET key '%s' must be included in the SELECT columns\n", required_key);
        return 0;
    }

    printf("  Success: Statement parsed cleanly.\n");
    return 1;
}

static int parse_insert(ParserState* state) {
    if (state->current_token.type != TOKEN_INSERT) {
        printf("Syntax Error: Expected INSERT near '%s'\n", state->current_token.text);
        return 0;
    }
    advance(state); // Consume INSERT

    if (state->current_token.type != TOKEN_IDENTIFIER) {
        printf("Syntax Error: Expected target table name near '%s'\n", state->current_token.text);
        return 0;
    }
    printf("  Target Table: [%s]\n", state->current_token.text);
    advance(state); // Consume table name

    if (state->current_token.type != TOKEN_VALUES) {
        printf("Syntax Error: Expected VALUES clause near '%s'\n", state->current_token.text);
        return 0;
    }
    advance(state); // Consume VALUES
    
    if (state->current_token.type != TOKEN_LPAREN) {
        printf("Syntax Error: Expected '(' opening values payload near '%s'\n", state->current_token.text);
        return 0;
    }
    advance(state); // Consume '('

    printf("  Values Inserted: ");
    if (state->current_token.type == TOKEN_RPAREN) {
        printf("\nSyntax Error: VALUES requires at least one constant\n");
        return 0;
    }
    while (1) {
        if (state->current_token.type != TOKEN_NUMBER && state->current_token.type != TOKEN_STRING) {
            printf("\nSyntax Error: Expected numeric or string constant near '%s'\n",
                   state->current_token.text);
            return 0;
        }
        printf(state->current_token.type == TOKEN_STRING ? "[\"%s\"] " : "[%s] ",
               state->current_token.text);
        advance(state);
        if (state->current_token.type == TOKEN_RPAREN) {
            advance(state);
            break;
        }
        if (state->current_token.type != TOKEN_COMMA) {
            printf("\nSyntax Error: Expected ',' or ')' after value near '%s'\n",
                   state->current_token.text);
            return 0;
        }
        advance(state);
    }
    printf("\n");

    if (!require_end_of_statement(state)) {
        return 0;
    }
    printf("Success: Statement parsed cleanly.\n");
    return 1;
}

static int parse_column_definition(ParserState* state) {
    if (state->current_token.type != TOKEN_IDENTIFIER) {
        printf("Syntax Error: Expected column name near '%s'\n", state->current_token.text);
        return 0;
    }
    printf("  Column Name: [%s] ", state->current_token.text);
    advance(state);

    if (state->current_token.type != TOKEN_TEXT &&
        state->current_token.type != TOKEN_INTEGER &&
        state->current_token.type != TOKEN_FLOAT) {
        printf("\nSyntax Error: Expected column type after column name near '%s'\n",
               state->current_token.text);
        return 0;
    }
    printf("Type: [%s]\n", state->current_token.text);
    advance(state);
    return 1;
}

static int parse_create_table(ParserState* state) {
    printf("Parsing CREATE TABLE statement:\n");
    if (state->current_token.type != TOKEN_IDENTIFIER) {
        printf("Syntax Error: Expected table name after CREATE TABLE near '%s'\n",
               state->current_token.text);
        return 0;
    }
    printf("  Table Name: [%s]\n", state->current_token.text);
    advance(state);

    if (state->current_token.type != TOKEN_LPAREN) {
        printf("Syntax Error: Expected '(' after table name near '%s'\n",
               state->current_token.text);
        return 0;
    }
    advance(state);

    if (!parse_column_definition(state)) {
        return 0;
    }
    if (strcasecmp(state->current_token.text, "PRIMARY") == 0) {
        advance(state);
        if (state->current_token.type != TOKEN_KEY) {
            printf("Syntax Error: Expected KEY after PRIMARY near '%s'\n",
                   state->current_token.text);
            return 0;
        }
        printf("  PRIMARY KEY designation found for column.\n");
        advance(state);
    }

    while (state->current_token.type == TOKEN_COMMA) {
        advance(state);
        if (!parse_column_definition(state)) {
            return 0;
        }
    }

    if (state->current_token.type != TOKEN_RPAREN) {
        printf("Syntax Error: Expected ',' or ')' after column definition near '%s'\n",
               state->current_token.text);
        return 0;
    }
    advance(state);
    return require_end_of_statement(state);
}

static int parse_create(ParserState* state) {
    if (state->current_token.type != TOKEN_CREATE) {
        printf("Syntax Error: Expected CREATE near '%s'\n", state->current_token.text);
        return 0;
    }
    advance(state);

    if (state->current_token.type == TOKEN_IDENTIFIER &&
        strcasecmp(state->current_token.text, "TABLE") == 0) {
        advance(state);
        return parse_create_table(state);
    }
    if (state->current_token.type == TOKEN_IDENTIFIER &&
        strcasecmp(state->current_token.text, "DATABASE") == 0) {
        advance(state);
        if (state->current_token.type != TOKEN_IDENTIFIER) {
            printf("Syntax Error: Expected database name after CREATE DATABASE near '%s'\n",
                   state->current_token.text);
            return 0;
        }
        printf("  Creating Database: [%s]\n", state->current_token.text);
        advance(state);
        return require_end_of_statement(state);
    }

    printf("Syntax Error: Expected DATABASE or TABLE after CREATE near '%s'\n",
           state->current_token.text);
    return 0;
}

static int parse_use(ParserState* state) {
    advance(state);
    if (state->current_token.type != TOKEN_IDENTIFIER) {
        printf("Syntax Error: Expected database name after USE near '%s'\n",
               state->current_token.text);
        return 0;
    }
    printf("Using Database: [%s]\n", state->current_token.text);
    advance(state);
    return require_end_of_statement(state);
}

static int parse_describe(ParserState* state) {
    advance(state);
    if (state->current_token.type != TOKEN_IDENTIFIER) {
        printf("Syntax Error: Expected ALL or table name after DESCRIBE near '%s'\n",
               state->current_token.text);
        return 0;
    }

    if (strcasecmp(state->current_token.text, "ALL") == 0) {
        printf("  Describing all tables in the database\n");
    } else {
        printf("  Table Name: [%s]\n", state->current_token.text);
    }
    advance(state);
    return require_end_of_statement(state);
}

static int parse_let(ParserState* state) {
    advance(state);
    if (state->current_token.type != TOKEN_IDENTIFIER) {
        printf("Syntax Error: Expected variable name after LET near '%s'\n",
               state->current_token.text);
        return 0;
    }

    printf("Storing under table name: [%s]\n", state->current_token.text);
    advance(state);
    if (state->current_token.type != TOKEN_KEY) {
        printf("Syntax Error: Expected KEY after variable name in LET statement near '%s'\n",
               state->current_token.text);
        return 0;
    }
    advance(state);
    if (state->current_token.type != TOKEN_IDENTIFIER) {
        printf("Syntax Error: Expected attribute name after KEY in LET statement near '%s'\n",
               state->current_token.text);
        return 0;
    }

    char required_key[64];
    strcpy(required_key, state->current_token.text);
    printf("Attribute Column Name: [%s]\n", required_key);
    advance(state);
    return parse_select(state, required_key);
}

static int parse_rename(ParserState* state) {
    if (state->current_token.type != TOKEN_RENAME) {
        printf("Syntax Error: Expected RENAME near '%s'\n", state->current_token.text);
        return 0;
    }
    advance(state);
    if (state->current_token.type != TOKEN_IDENTIFIER) {
        printf("Syntax Error: Expected old table name after RENAME near '%s'\n",
               state->current_token.text);
        return 0;
    }
    printf("  Old Table Name: [%s]\n", state->current_token.text);
    advance(state);

    if (state->current_token.type == TOKEN_LPAREN) {
        advance(state);
        if (state->current_token.type != TOKEN_IDENTIFIER) {
            printf("Syntax Error: Expected at least one new attribute name near '%s'\n",
                   state->current_token.text);
            return 0;
        }
        while (1) {
            printf("  New Attribute of Table Name: [%s]\n", state->current_token.text);
            advance(state);
            if (state->current_token.type == TOKEN_RPAREN) {
                advance(state);
                break;
            }
            if (state->current_token.type != TOKEN_COMMA) {
                printf("Syntax Error: Expected ',' or ')' in RENAME attribute list near '%s'\n",
                       state->current_token.text);
                return 0;
            }
            advance(state);
            if (state->current_token.type != TOKEN_IDENTIFIER) {
                printf("Syntax Error: Expected attribute name after ',' near '%s'\n",
                       state->current_token.text);
                return 0;
            }
        }
    }

    return require_end_of_statement(state);
}

static int parse_update(ParserState* state) {
    if (state->current_token.type != TOKEN_UPDATE) {
        printf("Syntax Error: Expected UPDATE near '%s'\n", state->current_token.text);
        return 0;
    }
    advance(state);
    if (state->current_token.type != TOKEN_IDENTIFIER) {
        printf("Syntax Error: Expected table name after UPDATE near '%s'\n",
               state->current_token.text);
        return 0;
    }
    printf("  Target Table: [%s]\n", state->current_token.text);
    advance(state);

    if (state->current_token.type != TOKEN_IDENTIFIER ||
        strcasecmp(state->current_token.text, "SET") != 0) {
        printf("Syntax Error: Expected SET after table name in UPDATE statement near '%s'\n",
               state->current_token.text);
        return 0;
    }
    advance(state);

    int assignment_count = 0;
    while (1) {
        if (state->current_token.type != TOKEN_IDENTIFIER) {
            printf("Syntax Error: Expected attribute name in UPDATE statement near '%s'\n",
                   state->current_token.text);
            return 0;
        }
        printf("  AttrName: [%s] ", state->current_token.text);
        advance(state);
        if (state->current_token.type != TOKEN_EQUAL) {
            printf("\nSyntax Error: Expected '=' after attribute name in UPDATE statement near '%s'\n",
                   state->current_token.text);
            return 0;
        }
        printf("[op:%s] ", state->current_token.text);
        advance(state);
        if (state->current_token.type != TOKEN_STRING && state->current_token.type != TOKEN_NUMBER) {
            printf("\nSyntax Error: Expected constant after '=' in UPDATE statement near '%s'\n",
                   state->current_token.text);
            return 0;
        }
        printf("  New Value: [%s]\n", state->current_token.text);
        advance(state);
        assignment_count++;

        if (state->current_token.type != TOKEN_COMMA) {
            break;
        }
        advance(state);
    }

    if (assignment_count == 0) {
        printf("Syntax Error: UPDATE requires at least one SET assignment\n");
        return 0;
    }
    if (state->current_token.type == TOKEN_WHERE && !parse_where(state)) {
        return 0;
    }
    return require_end_of_statement(state);
}

static int parse_delete(ParserState* state) {
    if (state->current_token.type != TOKEN_DELETE) {
        printf("Syntax Error: Expected DELETE near '%s'\n", state->current_token.text);
        return 0;
    }
    advance(state);
    if (state->current_token.type != TOKEN_IDENTIFIER) {
        printf("Syntax Error: Expected table name after DELETE near '%s'\n",
               state->current_token.text);
        return 0;
    }
    printf("  Target Table: [%s]\n", state->current_token.text);
    advance(state);
    if (state->current_token.type == TOKEN_WHERE && !parse_where(state)) {
        return 0;
    }
    return require_end_of_statement(state);
}

static int parse_input(ParserState* state) {
    if (state->current_token.type != TOKEN_INPUT) {
        printf("Syntax Error: Expected INPUT near '%s'\n", state->current_token.text);
        return 0;
    }
    advance(state);
    if ((state->current_token.type != TOKEN_IDENTIFIER &&
         state->current_token.type != TOKEN_FILENAME &&
         state->current_token.type != TOKEN_STRING) ||
        state->current_token.text[0] == '\0') {
        printf("Syntax Error: Expected input file name after INPUT near '%s'\n",
               state->current_token.text);
        return 0;
    }
    printf("  Input File: [%s]\n", state->current_token.text);
    advance(state);

    if (state->current_token.type == TOKEN_IDENTIFIER &&
        strcasecmp(state->current_token.text, "OUTPUT") == 0) {
        advance(state);
        if ((state->current_token.type != TOKEN_IDENTIFIER &&
             state->current_token.type != TOKEN_FILENAME &&
             state->current_token.type != TOKEN_STRING) ||
            state->current_token.text[0] == '\0') {
            printf("Syntax Error: Expected output file name after OUTPUT near '%s'\n",
                   state->current_token.text);
            return 0;
        }
        printf("  Output File: [%s]\n", state->current_token.text);
        advance(state);
    }
    return require_end_of_statement(state);
}

int statement_identifier(char *statement) {
    ParserState state;
    char command_buffer[128];
    char lower_command[32];

    state.source = statement;
    state.cursor = 0;
    printf("Identifying statement:%s\n", statement);

    snprintf(command_buffer, sizeof(command_buffer), "%s", statement);
    char *command = strtok(command_buffer, " \r\n");
    if (command == NULL) {
        printf("Empty statement\n");
        return 1;
    }

    size_t i;
    for (i = 0; command[i] != '\0' && i < sizeof(lower_command) - 1; i++) {
        lower_command[i] = (char)tolower((unsigned char)command[i]);
    }
    lower_command[i] = '\0';

    if (strcmp(lower_command, "create") == 0) {
        printf("Statement identified:  CREATE\n");
        advance(&state);
        parse_create(&state);
    } else if (strcmp(lower_command, "use") == 0) {
        printf("Statement identified:  USE\n");
        advance(&state);
        parse_use(&state);
    } else if (strcmp(lower_command, "describe") == 0 || strcmp(lower_command, "desc") == 0) {
        printf("Statement identified:  DESCRIBE\n");
        advance(&state);
        parse_describe(&state);
    } else if (strcmp(lower_command, "select") == 0) {
        printf("Statement identified:  SELECT\n");
        advance(&state);
        parse_select(&state, NULL);
    } else if (strcmp(lower_command, "let") == 0) {
        printf("Statement identified:  LET\n");
        advance(&state);
        parse_let(&state);
    } else if (strcmp(lower_command, "insert") == 0) {
        printf("Statement identified:  INSERT\n");
        advance(&state);
        parse_insert(&state);
    } else if (strcmp(lower_command, "update") == 0) {
        printf("Statement identified:  UPDATE\n");
        advance(&state);
        parse_update(&state);
    } else if (strcmp(lower_command, "delete") == 0) {
        printf("Statement identified:  DELETE\n");
        advance(&state);
        parse_delete(&state);
    } else if (strcmp(lower_command, "rename") == 0) {
        printf("Statement identified:  RENAME\n");
        advance(&state);
        parse_rename(&state);
    } else if (strcmp(lower_command, "input") == 0) {
        printf("Statement identified:  INPUT\n");
        advance(&state);
        parse_input(&state);
    } else if (strcmp(lower_command, "exit") == 0) {
        printf("Statement identified:  EXIT\n");
        advance(&state);
        advance(&state);
        if (require_end_of_statement(&state)) {
            printf("Exiting.\n");
            return 0;
        }
    } else if (strcmp(lower_command, "--") == 0) {
        // printf("Skipping comment\n");
    }
    else {
        printf("Unknown statement type: %s\n", command);
    }
    return 1;
}
