#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Define supported token types
typedef enum {
    TOKEN_SELECT,
    TOKEN_INSERT,
    TOKEN_INTO,
    TOKEN_FROM,
    TOKEN_VALUES,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_COMMA,
    TOKEN_ASTERISK,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

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

    // Numbers
    if (isdigit(current_char)) {
        size_t len = 0;
        while (isdigit(state->source[state->cursor])) {
            if (len < 63) token.text[len++] = state->source[state->cursor];
            state->cursor++;
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

        // Check if identifier matches standard SQL keywords
        if (strcasecmp(token.text, "SELECT") == 0) token.type = TOKEN_SELECT;
        else if (strcasecmp(token.text, "INSERT") == 0) token.type = TOKEN_INSERT;
        else if (strcasecmp(token.text, "INTO") == 0) token.type = TOKEN_INTO;
        else if (strcasecmp(token.text, "FROM") == 0) token.type = TOKEN_FROM;
        else if (strcasecmp(token.text, "VALUES") == 0) token.type = TOKEN_VALUES;
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
void parse_select(ParserState* state) {
    printf("Parsing SELECT statement:\n");
    advance(state); // Consume SELECT

    // Parse Columns
    printf("  Columns: ");
    while (1) {
        if (state->current_token.type == TOKEN_ASTERISK || state->current_token.type == TOKEN_IDENTIFIER) {
            printf("[%s] ", state->current_token.text);
            advance(state);
        } else {
            printf("\nSyntax Error: Expected column name or '*' near '%s'\n", state->current_token.text);
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
        if (state->current_token.type == TOKEN_IDENTIFIER) {
            printf("  From Table: [%s]\n", state->current_token.text);
            advance(state);
        } else {
            printf("Syntax Error: Expected table name after FROM near '%s'\n", state->current_token.text);
            return;
        }
    } else {
        printf("Syntax Error: Missing FROM clause near '%s'\n", state->current_token.text);
        return;
    }

    if (state->current_token.type == TOKEN_EOF) {
        printf("Success: Statement parsed cleanly.\n");
    } else {
        printf("Warning: Trailing unexpected tokens left over.\n");
    }
}

void parse_insert(ParserState* state) {
    printf("Parsing INSERT INTO statement:\n");
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
    }
}

// Master parser entry routing
void parse_sql(const char* sql) {
    ParserState state;
    state.source = sql;
    state.cursor = 0;
    
    printf("\nInput Query: \"%s\"\n", sql);
    advance(&state); // Seed initial token

    if (state.current_token.type == TOKEN_SELECT) {
        parse_select(&state);
    } else if (state.current_token.type == TOKEN_INSERT) {
        parse_insert(&state);
    } else {
        printf("Syntax Error: Unsupported keyword entry statement starting with '%s'\n", state.current_token.text);
    }
}

int main() {
    // Valid queries
    parse_sql("SELECT id, name, age FROM users");
    parse_sql("INSERT INTO logs VALUES (101, 200)");

    // Invalid query to demonstrate failure recovery
    parse_sql("SELECT FROM tables");

    return 0;
}
