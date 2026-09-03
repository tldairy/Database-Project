/*
 * sql_tokenizer.c
 *
 * A simple, dependency-free tokenizer (lexer) for SQL queries in C.
 *
 * Supports:
 *   - Keywords (SELECT, FROM, WHERE, INSERT, UPDATE, DELETE, JOIN, etc.)
 *   - Identifiers (unquoted and double-quoted / backtick-quoted)
 *   - Numeric literals (integers, decimals, exponents)
 *   - String literals ('...', with '' as an escaped quote)
 *   - Operators (=, <>, !=, <=, >=, <, >, +, -, *, /, %, ||)
 *   - Punctuation ( , ; ( ) . )
 *   - Line comments (-- ...) and block comments (slash-star ... star-slash)
 *   - Whitespace skipping
 *
 * Build:
 *   gcc -Wall -Wextra -std=c11 -o sql_tokenizer sql_tokenizer.c
 *
 * Run:
 *   ./sql_tokenizer "SELECT id, name FROM users WHERE age >= 18;"
 *   echo "SELECT 1" | ./sql_tokenizer
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  /* strcasecmp */
#include <ctype.h>

/* ---------------------------------------------------------------------- */
/* Token types                                                            */
/* ---------------------------------------------------------------------- */

typedef enum {
    TOK_EOF = 0,
    TOK_KEYWORD,
    TOK_IDENTIFIER,
    TOK_QUOTED_IDENTIFIER,
    TOK_NUMBER,
    TOK_STRING,
    TOK_OPERATOR,
    TOK_PUNCTUATION,
    TOK_UNKNOWN
} TokenType;

static const char *token_type_name(TokenType t) {
    switch (t) {
        case TOK_EOF:              return "EOF";
        case TOK_KEYWORD:          return "KEYWORD";
        case TOK_IDENTIFIER:       return "IDENTIFIER";
        case TOK_QUOTED_IDENTIFIER:return "QUOTED_IDENTIFIER";
        case TOK_NUMBER:           return "NUMBER";
        case TOK_STRING:           return "STRING";
        case TOK_OPERATOR:         return "OPERATOR";
        case TOK_PUNCTUATION:      return "PUNCTUATION";
        default:                   return "UNKNOWN";
    }
}

typedef struct {
    TokenType type;
    char *text;   /* heap-allocated, owned by the token */
    int line;
    int col;
} Token;

/* ---------------------------------------------------------------------- */
/* Keyword table                                                          */
/* ---------------------------------------------------------------------- */

static const char *KEYWORDS[] = {
    "SELECT", "FROM", "WHERE", "INSERT", "INTO", "VALUES", "UPDATE", "SET",
    "DELETE", "CREATE", "TABLE", "DROP", "ALTER", "ADD", "COLUMN",
    "JOIN", "INNER", "LEFT", "RIGHT", "FULL", "OUTER", "ON", "AS",
    "AND", "OR", "NOT", "NULL", "IS", "IN", "BETWEEN", "LIKE", "EXISTS",
    "GROUP", "BY", "ORDER", "HAVING", "LIMIT", "OFFSET", "DISTINCT",
    "UNION", "ALL", "ASC", "DESC", "PRIMARY", "KEY", "FOREIGN",
    "REFERENCES", "DEFAULT", "CHECK", "UNIQUE", "INDEX", "VIEW",
    "CASE", "WHEN", "THEN", "ELSE", "END", "TRUE", "FALSE",
    "BEGIN", "COMMIT", "ROLLBACK", "TRANSACTION",
    NULL
};

static int is_keyword(const char *word) {
    for (int i = 0; KEYWORDS[i] != NULL; i++) {
        if (strcasecmp(word, KEYWORDS[i]) == 0) return 1;
    }
    return 0;
}

/* ---------------------------------------------------------------------- */
/* Lexer state                                                            */
/* ---------------------------------------------------------------------- */

typedef struct {
    const char *src;
    size_t pos;
    size_t len;
    int line;
    int col;
} Lexer;

static void lexer_init(Lexer *lx, const char *src) {
    lx->src = src;
    lx->pos = 0;
    lx->len = strlen(src);
    lx->line = 1;
    lx->col = 1;
}

static int lx_peek(Lexer *lx) {
    return lx->pos < lx->len ? (unsigned char)lx->src[lx->pos] : '\0';
}

static int lx_peek_at(Lexer *lx, size_t offset) {
    size_t p = lx->pos + offset;
    return p < lx->len ? (unsigned char)lx->src[p] : '\0';
}

static int lx_advance(Lexer *lx) {
    int c = lx_peek(lx);
    if (c == '\0') return c;
    lx->pos++;
    if (c == '\n') {
        lx->line++;
        lx->col = 1;
    } else {
        lx->col++;
    }
    return c;
}

static Token *make_token(TokenType type, const char *start, size_t len,
                          int line, int col) {
    Token *tok = malloc(sizeof(Token));
    tok->type = type;
    tok->text = malloc(len + 1);
    memcpy(tok->text, start, len);
    tok->text[len] = '\0';
    tok->line = line;
    tok->col = col;
    return tok;
}

static void free_token(Token *tok) {
    if (!tok) return;
    free(tok->text);
    free(tok);
}

/* Skip whitespace and comments; returns 1 if anything was skipped. */
static void skip_ignorable(Lexer *lx) {
    for (;;) {
        int c = lx_peek(lx);

        if (isspace(c)) {
            lx_advance(lx);
            continue;
        }

        /* Line comment: -- ... end of line */
        if (c == '-' && lx_peek_at(lx, 1) == '-') {
            while (lx_peek(lx) != '\0' && lx_peek(lx) != '\n') lx_advance(lx);
            continue;
        }

        /* Block comment: slash-star ... star-slash */
        if (c == '/' && lx_peek_at(lx, 1) == '*') {
            lx_advance(lx); /* / */
            lx_advance(lx); /* * */
            while (lx_peek(lx) != '\0' &&
                   !(lx_peek(lx) == '*' && lx_peek_at(lx, 1) == '/')) {
                lx_advance(lx);
            }
            if (lx_peek(lx) != '\0') {
                lx_advance(lx); /* * */
                lx_advance(lx); /* / */
            }
            continue;
        }

        break;
    }
}

/* Reads a single quoted string literal starting at the opening quote `q`.
 * Handles doubled-quote escaping (e.g. '' inside '...'). */
static Token *read_quoted(Lexer *lx, char q, TokenType type) {
    int line = lx->line, col = lx->col;
    size_t start = lx->pos;
    lx_advance(lx); /* consume opening quote */

    for (;;) {
        int c = lx_peek(lx);
        if (c == '\0') break; /* unterminated */
        if (c == q) {
            if (lx_peek_at(lx, 1) == q) {
                lx_advance(lx); /* first quote */
                lx_advance(lx); /* second quote (escaped) */
                continue;
            }
            lx_advance(lx); /* closing quote */
            break;
        }
        lx_advance(lx);
    }

    size_t len = lx->pos - start;
    return make_token(type, lx->src + start, len, line, col);
}

static Token *read_number(Lexer *lx) {
    int line = lx->line, col = lx->col;
    size_t start = lx->pos;

    while (isdigit(lx_peek(lx))) lx_advance(lx);

    if (lx_peek(lx) == '.' && isdigit(lx_peek_at(lx, 1))) {
        lx_advance(lx); /* . */
        while (isdigit(lx_peek(lx))) lx_advance(lx);
    }

    if (lx_peek(lx) == 'e' || lx_peek(lx) == 'E') {
        size_t save = lx->pos;
        int sline = lx->line, scol = lx->col;
        lx_advance(lx);
        if (lx_peek(lx) == '+' || lx_peek(lx) == '-') lx_advance(lx);
        if (isdigit(lx_peek(lx))) {
            while (isdigit(lx_peek(lx))) lx_advance(lx);
        } else {
            /* not a valid exponent, rewind */
            lx->pos = save;
            lx->line = sline;
            lx->col = scol;
        }
    }

    size_t len = lx->pos - start;
    return make_token(TOK_NUMBER, lx->src + start, len, line, col);
}

static Token *read_word(Lexer *lx) {
    int line = lx->line, col = lx->col;
    size_t start = lx->pos;

    while (isalnum(lx_peek(lx)) || lx_peek(lx) == '_') lx_advance(lx);

    size_t len = lx->pos - start;
    char *word = malloc(len + 1);
    memcpy(word, lx->src + start, len);
    word[len] = '\0';

    TokenType type = is_keyword(word) ? TOK_KEYWORD : TOK_IDENTIFIER;
    Token *tok = make_token(type, lx->src + start, len, line, col);
    free(word);
    return tok;
}

/* Reads operators, preferring the longest match. */
static Token *read_operator(Lexer *lx) {
    int line = lx->line, col = lx->col;
    int c = lx_peek(lx);
    int c2 = lx_peek_at(lx, 1);

    /* two-char operators */
    if ((c == '<' && c2 == '>') ||
        (c == '<' && c2 == '=') ||
        (c == '>' && c2 == '=') ||
        (c == '!' && c2 == '=') ||
        (c == '|' && c2 == '|')) {
        size_t start = lx->pos;
        lx_advance(lx);
        lx_advance(lx);
        return make_token(TOK_OPERATOR, lx->src + start, 2, line, col);
    }

    /* single-char operators */
    if (strchr("=<>+-*/%", c)) {
        size_t start = lx->pos;
        lx_advance(lx);
        return make_token(TOK_OPERATOR, lx->src + start, 1, line, col);
    }

    return NULL;
}

static Token *next_token(Lexer *lx) {
    skip_ignorable(lx);

    int line = lx->line, col = lx->col;
    int c = lx_peek(lx);

    if (c == '\0') {
        return make_token(TOK_EOF, "", 0, line, col);
    }

    /* String literal */
    if (c == '\'') {
        return read_quoted(lx, '\'', TOK_STRING);
    }

    /* Double-quoted or backtick-quoted identifier */
    if (c == '"') {
        return read_quoted(lx, '"', TOK_QUOTED_IDENTIFIER);
    }
    if (c == '`') {
        return read_quoted(lx, '`', TOK_QUOTED_IDENTIFIER);
    }

    /* Number */
    if (isdigit(c)) {
        return read_number(lx);
    }

    /* Identifier / keyword */
    if (isalpha(c) || c == '_') {
        return read_word(lx);
    }

    /* Operators */
    Token *op = read_operator(lx);
    if (op) return op;

    /* Punctuation */
    if (strchr(",;().", c)) {
        size_t start = lx->pos;
        lx_advance(lx);
        return make_token(TOK_PUNCTUATION, lx->src + start, 1, line, col);
    }

    /* Unknown single character */
    {
        size_t start = lx->pos;
        lx_advance(lx);
        return make_token(TOK_UNKNOWN, lx->src + start, 1, line, col);
    }
}

/* ---------------------------------------------------------------------- */
/* Driver                                                                 */
/* ---------------------------------------------------------------------- */

static char *read_all_stdin(void) {
    size_t cap = 4096, len = 0;
    char *buf = malloc(cap);
    int c;
    while ((c = fgetc(stdin)) != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
        buf[len++] = (char)c;
    }
    buf[len] = '\0';
    return buf;
}

static void tokenize_and_print(const char *sql) {
    Lexer lx;
    lexer_init(&lx, sql);

    printf("%-4s %-4s %-18s %s\n", "LN", "COL", "TYPE", "TEXT");
    printf("---- ---- ------------------ ----------------------------\n");

    for (;;) {
        Token *tok = next_token(&lx);
        if (tok->type == TOK_EOF) {
            free_token(tok);
            break;
        }
        printf("%-4d %-4d %-18s %s\n",
               tok->line, tok->col, token_type_name(tok->type), tok->text);
        free_token(tok);
    }
}

int main(int argc, char **argv) {
    char *sql = NULL;
    int owns_sql = 0;

    if (argc > 1) {
        /* Join all args after argv[0] with spaces, in case the query
         * wasn't wrapped in quotes on the command line. */
        size_t total = 0;
        for (int i = 1; i < argc; i++) total += strlen(argv[i]) + 1;
        sql = malloc(total + 1);
        sql[0] = '\0';
        for (int i = 1; i < argc; i++) {
            strcat(sql, argv[i]);
            if (i != argc - 1) strcat(sql, " ");
        }
        owns_sql = 1;
    } else {
        sql = read_all_stdin();
        owns_sql = 1;
    }

    tokenize_and_print(sql);

    if (owns_sql) free(sql);
    return 0;
}