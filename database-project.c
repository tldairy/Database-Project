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

    // for (int i = 0; i < *num_tokens; i++) {
    //     printf("Token %d: %s\n", i + 1, tokens[i]);
    // }
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

    
    TOKEN_INTO,
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
    TOKEN_EOF,
    TOKEN_ERROR,


    //Tokens for column data types
    TOKEN_TEXT,
    TOKEN_INTEGER,
    TOKEN_FLOAT,
} TokenType;

void print_token_type(TokenType type) {
    switch (type) {
        case TOKEN_CREATE: printf("TOKEN_CREATE"); break;
        case TOKEN_USE: printf("TOKEN_USE"); break;
        case TOKEN_DESCRIBE: printf("TOKEN_DESCRIBE"); break;
        case TOKEN_SELECT: printf("TOKEN_SELECT"); break;
        case TOKEN_LET: printf("TOKEN_LET"); break;
        case TOKEN_INSERT: printf("TOKEN_INSERT"); break;
        case TOKEN_UPDATE: printf("TOKEN_UPDATE"); break;
        case TOKEN_DELETE: printf("TOKEN_DELETE"); break;
        case TOKEN_RENAME: printf("TOKEN_RENAME"); break;
        case TOKEN_INPUT: printf("TOKEN_INPUT"); break;
        case TOKEN_EXIT: printf("TOKEN_EXIT"); break;
        case TOKEN_INTO: printf("TOKEN_INTO"); break;
        case TOKEN_FROM: printf("TOKEN_FROM"); break;
        case TOKEN_VALUES: printf("TOKEN_VALUES"); break;
        case TOKEN_WHERE: printf("TOKEN_WHERE"); break;
        case TOKEN_AND: printf("TOKEN_AND"); break;
        case TOKEN_OR: printf("TOKEN_OR"); break;
        case TOKEN_IDENTIFIER: printf("TOKEN_IDENTIFIER"); break;
        case TOKEN_NUMBER: printf("TOKEN_NUMBER"); break;
        case TOKEN_STRING: printf("TOKEN_STRING"); break;
        case TOKEN_COMMA: printf("TOKEN_COMMA"); break;
        case TOKEN_ASTERISK: printf("TOKEN_ASTERISK"); break;
        case TOKEN_LPAREN: printf("TOKEN_LPAREN"); break;
        case TOKEN_RPAREN: printf("TOKEN_RPAREN"); break;
        case TOKEN_EQUAL: printf("TOKEN_EQUAL"); break;
        case TOKEN_NOT_EQUAL: printf("TOKEN_NOT_EQUAL"); break;
        case TOKEN_GREATER: printf("TOKEN_GREATER"); break;
        case TOKEN_LESS: printf("TOKEN_LESS"); break;
        case TOKEN_GREATER_EQUAL: printf("TOKEN_GREATER_EQUAL"); break;
        case TOKEN_LESS_EQUAL: printf("TOKEN_LESS_EQUAL"); break;
        case TOKEN_EOF: printf("TOKEN_EOF"); break;
        case TOKEN_ERROR: printf("TOKEN_ERROR"); break;
    }
}
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
        if (state->source[state->cursor] == '"') {
            state->cursor++;
        }
        token.type = TOKEN_STRING;
        return token;
    }

    // Numbers
    if (isdigit(current_char)) {
        size_t len = 0;
        while (isdigit(state->source[state->cursor])) {
            if (len < 63) token.text[len++] = state->source[state->cursor];
            state->cursor++;
        }
        if (state->source[state->cursor] == '.') {
            if (len < 63) token.text[len++] = state->source[state->cursor];
            state->cursor++;
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
        else if (strcasecmp(token.text, "INTO") == 0) token.type = TOKEN_INTO;
        else if (strcasecmp(token.text, "FROM") == 0) token.type = TOKEN_FROM;
        else if (strcasecmp(token.text, "VALUES") == 0) token.type = TOKEN_VALUES;
        else if (strcasecmp(token.text, "WHERE") == 0) token.type = TOKEN_WHERE;
        else if (strcasecmp(token.text, "AND") == 0) token.type = TOKEN_AND;
        else if (strcasecmp(token.text, "OR") == 0) token.type = TOKEN_OR;
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
static int parse_select_item(ParserState* state) {
    if (state->current_token.type == TOKEN_ASTERISK) {
        printf("[%s] ", state->current_token.text);
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
        advance(state);
        if (state->current_token.type == TOKEN_ASTERISK || state->current_token.type == TOKEN_IDENTIFIER) {
            printf("[%s(%s)] ", func_name, state->current_token.text);
            advance(state);
            if (state->current_token.type != TOKEN_RPAREN) {
                printf("\nSyntax Error: Missing ')' in aggregate call near '%s'\n", state->current_token.text);
                return 0;
            }
            advance(state);
            return 1;
        }

        printf("\nSyntax Error: Expected aggregate argument near '%s'\n", state->current_token.text);
        return 0;
    }

    printf("[%s] ", func_name);
    return 1;
}

static int is_comparison_operator(TokenType type) {
    return type == TOKEN_EQUAL || type == TOKEN_NOT_EQUAL || type == TOKEN_GREATER ||
           type == TOKEN_LESS || type == TOKEN_GREATER_EQUAL || type == TOKEN_LESS_EQUAL;
}

void parse_where(ParserState* state) {
    if (state->current_token.type == TOKEN_WHERE) {
        advance(state); // Consume WHERE
    }

    while (state->current_token.type != TOKEN_EOF) {
        if (state->current_token.type == TOKEN_AND || state->current_token.type == TOKEN_OR) {
            printf("  Logic: [%s] ", state->current_token.text);
            advance(state);
            continue;
        }

        if (state->current_token.type == TOKEN_IDENTIFIER || state->current_token.type == TOKEN_NUMBER || state->current_token.type == TOKEN_STRING) {
            printf("  Condition: [%s] ", state->current_token.text);
            advance(state);

            if (!is_comparison_operator(state->current_token.type)) {
                printf("\nSyntax Error: Missing comparison operator near '%s'\n", state->current_token.text);
                return;
            }

            printf("[op:%s] ", state->current_token.text);
            advance(state);

            if (state->current_token.type != TOKEN_IDENTIFIER && state->current_token.type != TOKEN_NUMBER && state->current_token.type != TOKEN_STRING) {
                printf("\nSyntax Error: Missing value in WHERE clause near '%s'\n", state->current_token.text);
                return;
            }

            printf("[%s] ", state->current_token.text);
            advance(state);
            continue;
        }

        printf("\nSyntax Error: Unexpected token in WHERE clause near '%s'\n", state->current_token.text);
        return;
    }
}

void parse_select(ParserState* state) {
    printf("Parsing SELECT statement:\n");

    if (state->current_token.type != TOKEN_SELECT) {
        printf("Syntax Error: Expected SELECT near '%s'\n", state->current_token.text);
        return;
    }
    advance(state); // Consume SELECT

    // Parse Columns
    printf("  Columns: ");
    while (1) {
        if (!parse_select_item(state)) {
            printf("\nSyntax Error: Expected column name, '*' or aggregate function near '%s'\n", state->current_token.text);
            return;
        }

        if (state->current_token.type == TOKEN_COMMA) {
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
                return;
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
        return;
    }

    // Parse WHERE keyword
    if (state->current_token.type == TOKEN_WHERE) {
        parse_where(state);
    }

    if (state->current_token.type == TOKEN_EOF) {
        printf("Success: Statement parsed cleanly.\n");
    } else {
        printf("Warning: Trailing unexpected tokens left over.\n");
        printf("  Remaining tokens: ");
        for (int i = 0; i < 5 && state->current_token.type != TOKEN_EOF; i++) {
            printf("[%s]", state->current_token.text);
            advance(state);
        }
    }
}

void parse_insert(ParserState* state) {
    if (state->current_token.type != TOKEN_INSERT) {
        printf("Syntax Error: Expected INSERT near '%s'\n", state->current_token.text);
        return;
    }
    advance(state); // Consume INSERT

    if (state->current_token.type != TOKEN_INTO) {
        printf("Syntax Error: Expected INTO after INSERT near '%s'\n", state->current_token.text);
        return;
    }
    advance(state); // Consume INTO

    if (state->current_token.type != TOKEN_IDENTIFIER) {
        printf("Syntax Error: Expected target table name near '%s'\n", state->current_token.text);
        return;
    }
    printf("  Target Table: [%s]\n", state->current_token.text);
    advance(state); // Consume table name

    if (state->current_token.type != TOKEN_VALUES) {
        printf("Syntax Error: Expected VALUES clause near '%s'\n", state->current_token.text);
        return;
    }
    advance(state); // Consume VALUES

    if (state->current_token.type != TOKEN_LPAREN) {
        printf("Syntax Error: Expected '(' opening values payload near '%s'\n", state->current_token.text);
        return;
    }
    advance(state); // Consume '('

    printf("  Values Inserted: ");
    while (1) {
        if (state->current_token.type == TOKEN_NUMBER || state->current_token.type == TOKEN_IDENTIFIER) {
            printf("[%s] ", state->current_token.text);
            advance(state);
        } else {
            printf("\nSyntax Error: Expected continuous value entry near '%s'\n", state->current_token.text);
            return;
        }

        if (state->current_token.type == TOKEN_COMMA) {
            advance(state);
        } else {
            break;
        }
    }
    printf("\n");

    if (state->current_token.type != TOKEN_RPAREN) {
        printf("Syntax Error: Missing close parenthesis ')' payload container near '%s'\n", state->current_token.text);
        return;
    }
    advance(state); // Consume ')'

    if (state->current_token.type == TOKEN_EOF) {
        printf("Success: Statement parsed cleanly.\n");
    } else {
        printf("Warning: Trailing unexpected tokens left over.\n");
        printf("  Remaining tokens: ");
        while (state->current_token.type != TOKEN_EOF) {
            printf("[%s] ", state->current_token.text);
            advance(state);
        }
        printf("\n");
    }
}

void parse_create_table (ParserState* state) {
    printf("Parsing CREATE TABLE statement:\n");
    if (state->current_token.type == TOKEN_IDENTIFIER) {
        printf("  Table Name: [%s]\n", state->current_token.text);
        advance(state); // Consume table name
    } else {
        printf("Syntax Error: Expected table name after CREATE TABLE near '%s'\n", state->current_token.text);
        return;
    }

    if (state->current_token.type != TOKEN_LPAREN) {
        printf("Syntax Error: Expected '(' after table name near '%s'\n", state->current_token.text);
        return;
    }
    advance(state); // Consume '('

    while (state->current_token.type != TOKEN_RPAREN && state->current_token.type != TOKEN_EOF) {
        if (state->current_token.type == TOKEN_IDENTIFIER) {
            printf("  Column Name: [%s] ", state->current_token.text);
            advance(state); // Consume column name

            if (state->current_token.type == TOKEN_IDENTIFIER) {
                printf("Type: [%s]\n", state->current_token.text);
                advance(state); // Consume column type
            } else {
                printf("\nSyntax Error: Expected column type after column name near '%s'\n", state->current_token.text);
                return;
            }

            if (state->current_token.type == TOKEN_COMMA) {
                advance(state); // Consume comma and continue with more columns
            }
        } else {
            printf("\nSyntax Error: Expected column definition near '%s'\n", state->current_token.text);
            return;
        }
    }
}

void parse_create (ParserState* state) {
    if (state->current_token.type != TOKEN_CREATE){
        printf("Syntax Error: Expected CREATE near '%s'\n", state->current_token.text);
        return;
    }
    advance(state); // Consume CREATE

    if (strcasecmp(state->current_token.text, "table") == 0) {
        advance(state); // Consume TABLE
        parse_create_table(state);
    } else {
        printf("Syntax Error: Expected TABLE after CREATE near '%s'\n", state->current_token.text);
        return;
    }

    if (state->current_token.type != TOKEN_IDENTIFIER) {
        printf("Syntax Error: Expected target table name near '%s'\n", state->current_token.text);
        return;
    }
    while (state->current_token.type != TOKEN_EOF) {
        if (state->current_token.type == TOKEN_IDENTIFIER) {
            printf("[%s] ", state->current_token.text);
            advance(state);
        } else {
            printf("\nSyntax Error: Expected table name near '%s'\n", state->current_token.text);
            return;
        }
    }

}

void statement_identifier(char *statement) {
    ParserState state;
    char command_buffer[128];
    char lower_command[32];

    state.source = statement;
    state.cursor = 0;
    printf("Identifying statement:\t\t%s\n", statement);

    snprintf(command_buffer, sizeof(command_buffer), "%s", statement);
    char *command = strtok(command_buffer, " \t\r\n");
    if (command == NULL) {
        printf("Empty statement\n");
        return;
    }

    size_t i;
    for (i = 0; command[i] != '\0' && i < sizeof(lower_command) - 1; i++) {
        lower_command[i] = (char)tolower((unsigned char)command[i]);
    }
    lower_command[i] = '\0';

    if (strcmp(lower_command, "create") == 0) {
        printf("\t\tCREATE statement identified\n");
        advance(&state);
        parse_create(&state);
    } else if (strcmp(lower_command, "use") == 0) {
        printf("\t\tUSE statement identified\n");
    } else if (strcmp(lower_command, "describe") == 0 || strcmp(lower_command, "desc") == 0) {
        printf("\t\tDESCRIBE statement identified\n");
    } else if (strcmp(lower_command, "select") == 0) {
        printf("\t\tSELECT statement identified\n");
        advance(&state);
        parse_select(&state);
    } else if (strcmp(lower_command, "let") == 0) {
        printf("\t\tLET statement identified\n");
    } else if (strcmp(lower_command, "insert") == 0) {
        printf("\t\tINSERT statement identified\n");
        advance(&state);
        parse_insert(&state);
    } else if (strcmp(lower_command, "update") == 0) {
        printf("\t\tUPDATE statement identified\n");
    } else if (strcmp(lower_command, "delete") == 0) {
        printf("\t\tDELETE statement identified\n");
    } else if (strcmp(lower_command, "rename") == 0) {
        printf("\t\tRENAME statement identified\n");
    } else if (strcmp(lower_command, "input") == 0) {
        printf("\t\tINPUT statement identified\n");
    } else if (strcmp(lower_command, "exit") == 0) {
        printf("\t\tEXIT statement identified\n");
    } else {
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
        printf("Statement number: %d\n", i + 1);
        statement_identifier(separated_statements[i]);
    }

    return 0;
}