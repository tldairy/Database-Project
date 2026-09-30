#include "semantic_analyzer.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEM_MAX_DATABASES 16
#define SEM_MAX_TABLES 64
#define SEM_MAX_COLUMNS 32
#define SEM_MAX_TOKENS 512
#define SEM_TOKEN_LENGTH 128

typedef enum {
    VALUE_TEXT,
    VALUE_INTEGER,
    VALUE_FLOAT
} ValueType;

typedef enum {
    TOK_WORD,
    TOK_NUMBER,
    TOK_STRING,
    TOK_SYMBOL
} SemanticTokenType;

typedef struct {
    SemanticTokenType type;
    char text[SEM_TOKEN_LENGTH];
} SemanticToken;

typedef struct {
    char name[SEM_TOKEN_LENGTH];
    ValueType type;
    int is_primary_key;
} SemanticColumn;

typedef struct {
    char **values;
} SemanticRow;

typedef struct {
    char name[SEM_TOKEN_LENGTH];
    size_t column_count;
    SemanticColumn columns[SEM_MAX_COLUMNS];
    SemanticRow *rows;
    size_t row_count;
    size_t row_capacity;
} SemanticTable;

typedef struct {
    char name[SEM_TOKEN_LENGTH];
    size_t table_count;
    SemanticTable tables[SEM_MAX_TABLES];
} SemanticDatabase;

typedef struct {
    SemanticToken tokens[SEM_MAX_TOKENS];
    size_t count;
    size_t position;
} SemanticParser;

static SemanticDatabase databases[SEM_MAX_DATABASES];
static size_t database_count;
static size_t active_database;
static int catalog_initialized;

static int text_equal(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0') {
        if (tolower((unsigned char)*left) != tolower((unsigned char)*right)) {
            return 0;
        }
        left++;
        right++;
    }
    return *left == '\0' && *right == '\0';
}

static void copy_text(char *destination, size_t capacity, const char *source)
{
    if (capacity == 0) {
        return;
    }
    snprintf(destination, capacity, "%s", source);
}

static void free_table_rows(SemanticTable *table)
{
    size_t row_index;
    size_t column_index;

    for (row_index = 0; row_index < table->row_count; row_index++) {
        for (column_index = 0; column_index < table->column_count; column_index++) {
            free(table->rows[row_index].values[column_index]);
        }
        free(table->rows[row_index].values);
    }
    free(table->rows);
    table->rows = NULL;
    table->row_count = 0;
    table->row_capacity = 0;
}

void semantic_analyzer_reset(void)
{
    size_t database_index;
    size_t table_index;

    for (database_index = 0; database_index < database_count; database_index++) {
        for (table_index = 0; table_index < databases[database_index].table_count; table_index++) {
            free_table_rows(&databases[database_index].tables[table_index]);
        }
    }

    memset(databases, 0, sizeof(databases));
    copy_text(databases[0].name, sizeof(databases[0].name), "default");
    database_count = 1;
    active_database = 0;
    catalog_initialized = 1;
}

static void ensure_initialized(void)
{
    if (!catalog_initialized) {
        semantic_analyzer_reset();
    }
}

static int is_identifier_start(char character)
{
    return isalpha((unsigned char)character) || character == '_';
}

static int is_identifier_part(char character)
{
    return isalnum((unsigned char)character) || character == '_';
}

static int tokenize_statement(const char *source, SemanticParser *parser)
{
    size_t cursor = 0;

    parser->count = 0;
    parser->position = 0;

    while (source[cursor] != '\0') {
        SemanticToken token;
        size_t length = 0;
        char character = source[cursor];

        if (isspace((unsigned char)character)) {
            cursor++;
            continue;
        }
        if (character == '-' && source[cursor + 1] == '-') {
            break;
        }
        if (parser->count >= SEM_MAX_TOKENS) {
            printf("Semantic error: statement has too many tokens.\n");
            return 0;
        }

        memset(&token, 0, sizeof(token));
        if (character == '"') {
            cursor++;
            while (source[cursor] != '\0' && source[cursor] != '"') {
                if (length + 1 < sizeof(token.text)) {
                    token.text[length++] = source[cursor];
                }
                cursor++;
            }
            if (source[cursor] != '"') {
                printf("Semantic error: unterminated string literal.\n");
                return 0;
            }
            cursor++;
            token.type = TOK_STRING;
        } else if (is_identifier_start(character)) {
            while (is_identifier_part(source[cursor])) {
                if (length + 1 < sizeof(token.text)) {
                    token.text[length++] = source[cursor];
                }
                cursor++;
            }
            token.type = TOK_WORD;
        } else if (isdigit((unsigned char)character) ||
                   (character == '.' && isdigit((unsigned char)source[cursor + 1]))) {
            int decimal_point = 0;
            while (isdigit((unsigned char)source[cursor]) ||
                   (source[cursor] == '.' && !decimal_point)) {
                if (source[cursor] == '.') {
                    decimal_point = 1;
                }
                if (length + 1 < sizeof(token.text)) {
                    token.text[length++] = source[cursor];
                }
                cursor++;
            }
            token.type = TOK_NUMBER;
        } else if (character == '=' || character == '!' ||
                   character == '<' || character == '>') {
            token.text[length++] = character;
            cursor++;
            if ((character == '!' || character == '<' || character == '>') &&
                source[cursor] == '=') {
                token.text[length++] = source[cursor++];
            }
            token.type = TOK_SYMBOL;
        } else if (character == ',' || character == '(' || character == ')' ||
                   character == '.' || character == '*') {
            token.text[length++] = character;
            cursor++;
            token.type = TOK_SYMBOL;
        } else {
            printf("Semantic error: unsupported character '%c'.\n", character);
            return 0;
        }

        token.text[length] = '\0';
        parser->tokens[parser->count++] = token;
    }
    return 1;
}

static SemanticToken *current_token(SemanticParser *parser)
{
    static SemanticToken end_token = { TOK_SYMBOL, "" };
    return parser->position < parser->count ? &parser->tokens[parser->position] : &end_token;
}

static int is_word(SemanticParser *parser, const char *word)
{
    SemanticToken *token = current_token(parser);
    return token->type == TOK_WORD && text_equal(token->text, word);
}

static int accept_word(SemanticParser *parser, const char *word)
{
    if (!is_word(parser, word)) {
        return 0;
    }
    parser->position++;
    return 1;
}

static int accept_symbol(SemanticParser *parser, const char *symbol)
{
    SemanticToken *token = current_token(parser);
    if (token->type != TOK_SYMBOL || strcmp(token->text, symbol) != 0) {
        return 0;
    }
    parser->position++;
    return 1;
}

static int expect_word(SemanticParser *parser, char *output, size_t capacity, const char *description)
{
    SemanticToken *token = current_token(parser);
    if (token->type != TOK_WORD) {
        printf("Semantic error: expected %s near '%s'.\n", description, token->text);
        return 0;
    }
    copy_text(output, capacity, token->text);
    parser->position++;
    return 1;
}

static int expect_end(SemanticParser *parser)
{
    if (parser->position != parser->count) {
        printf("Semantic error: unexpected token '%s'.\n", current_token(parser)->text);
        return 0;
    }
    return 1;
}

static SemanticDatabase *active_db(void)
{
    ensure_initialized();
    return &databases[active_database];
}

static int find_database(const char *name)
{
    size_t index;
    for (index = 0; index < database_count; index++) {
        if (text_equal(databases[index].name, name)) {
            return (int)index;
        }
    }
    return -1;
}

static int find_table_index(SemanticDatabase *database, const char *name)
{
    size_t index;
    for (index = 0; index < database->table_count; index++) {
        if (text_equal(database->tables[index].name, name)) {
            return (int)index;
        }
    }
    return -1;
}

static SemanticTable *find_table(SemanticDatabase *database, const char *name)
{
    int index = find_table_index(database, name);
    return index < 0 ? NULL : &database->tables[index];
}

static int find_column_index(const SemanticTable *table, const char *name)
{
    size_t index;
    for (index = 0; index < table->column_count; index++) {
        if (text_equal(table->columns[index].name, name)) {
            return (int)index;
        }
    }
    return -1;
}

static int column_type_from_name(const char *name, ValueType *type)
{
    if (text_equal(name, "text")) {
        *type = VALUE_TEXT;
        return 1;
    }
    if (text_equal(name, "integer")) {
        *type = VALUE_INTEGER;
        return 1;
    }
    if (text_equal(name, "float")) {
        *type = VALUE_FLOAT;
        return 1;
    }
    return 0;
}

static const char *type_name(ValueType type)
{
    switch (type) {
    case VALUE_TEXT:
        return "text";
    case VALUE_INTEGER:
        return "integer";
    case VALUE_FLOAT:
        return "float";
    }
    return "unknown";
}

static int token_type_matches(const SemanticToken *token, ValueType expected)
{
    if (expected == VALUE_TEXT) {
        return token->type == TOK_STRING;
    }
    if (token->type != TOK_NUMBER) {
        return 0;
    }
    if (expected == VALUE_INTEGER) {
        return strchr(token->text, '.') == NULL;
    }
    return 1;
}

static int numeric_values_equal(const char *left, const char *right)
{
    char *left_end;
    char *right_end;
    double left_value = strtod(left, &left_end);
    double right_value = strtod(right, &right_end);
    return *left_end == '\0' && *right_end == '\0' && left_value == right_value;
}

static int values_equal(const char *left, const char *right, ValueType type)
{
    if (type == VALUE_TEXT) {
        return strcmp(left, right) == 0;
    }
    return numeric_values_equal(left, right);
}

static int duplicate_primary_key(const SemanticTable *table, const char *value, size_t except_row)
{
    size_t row;
    int key_column = -1;
    size_t column;

    for (column = 0; column < table->column_count; column++) {
        if (table->columns[column].is_primary_key) {
            key_column = (int)column;
            break;
        }
    }
    if (key_column < 0) {
        return 0;
    }
    for (row = 0; row < table->row_count; row++) {
        if (row != except_row &&
            values_equal(table->rows[row].values[key_column], value,
                         table->columns[key_column].type)) {
            return 1;
        }
    }
    return 0;
}

static int append_row(SemanticTable *table, const SemanticToken *values, size_t count)
{
    SemanticRow row;
    size_t column;

    if (table->row_count == table->row_capacity) {
        size_t new_capacity = table->row_capacity == 0 ? 8 : table->row_capacity * 2;
        SemanticRow *new_rows = realloc(table->rows, new_capacity * sizeof(*new_rows));
        if (new_rows == NULL) {
            printf("Semantic error: unable to allocate row storage.\n");
            return 0;
        }
        table->rows = new_rows;
        table->row_capacity = new_capacity;
    }

    row.values = calloc(table->column_count, sizeof(*row.values));
    if (row.values == NULL) {
        printf("Semantic error: unable to allocate row storage.\n");
        return 0;
    }
    for (column = 0; column < count; column++) {
        row.values[column] = malloc(strlen(values[column].text) + 1);
        if (row.values[column] == NULL) {
            size_t cleanup;
            for (cleanup = 0; cleanup < column; cleanup++) {
                free(row.values[cleanup]);
            }
            free(row.values);
            printf("Semantic error: unable to allocate row value.\n");
            return 0;
        }
        strcpy(row.values[column], values[column].text);
    }
    table->rows[table->row_count++] = row;
    return 1;
}

static int parse_create_database(SemanticParser *parser)
{
    char name[SEM_TOKEN_LENGTH];
    if (!expect_word(parser, name, sizeof(name), "a database name") ||
        !expect_end(parser)) {
        return 0;
    }
    if (find_database(name) >= 0) {
        printf("Semantic error: database '%s' already exists.\n", name);
        return 0;
    }
    if (database_count == SEM_MAX_DATABASES) {
        printf("Semantic error: database catalog is full.\n");
        return 0;
    }
    copy_text(databases[database_count].name, sizeof(databases[database_count].name), name);
    database_count++;
    return 1;
}

static int parse_create_table(SemanticParser *parser)
{
    SemanticDatabase *database = active_db();
    SemanticTable candidate;
    char name[SEM_TOKEN_LENGTH];

    if (!expect_word(parser, name, sizeof(name), "a table name")) {
        return 0;
    }
    if (find_table(database, name) != NULL) {
        printf("Semantic error: table '%s' already exists in database '%s'.\n",
               name, database->name);
        return 0;
    }
    if (database->table_count == SEM_MAX_TABLES) {
        printf("Semantic error: table catalog is full for database '%s'.\n", database->name);
        return 0;
    }
    if (!accept_symbol(parser, "(")) {
        printf("Semantic error: expected '(' after table name '%s'.\n", name);
        return 0;
    }

    memset(&candidate, 0, sizeof(candidate));
    copy_text(candidate.name, sizeof(candidate.name), name);
    do {
        SemanticColumn *column;
        char column_name[SEM_TOKEN_LENGTH];
        ValueType type;

        if (candidate.column_count == SEM_MAX_COLUMNS) {
            printf("Semantic error: table '%s' has too many columns.\n", name);
            return 0;
        }
        if (!expect_word(parser, column_name, sizeof(column_name), "a column name")) {
            return 0;
        }
        if (find_column_index(&candidate, column_name) >= 0) {
            printf("Semantic error: column '%s' is declared more than once in table '%s'.\n",
                   column_name, name);
            return 0;
        }
        if (!expect_word(parser, name, sizeof(name), "a column type")) {
            return 0;
        }
        if (!column_type_from_name(name, &type)) {
            printf("Semantic error: unsupported type '%s' for column '%s'.\n", name, column_name);
            return 0;
        }

        column = &candidate.columns[candidate.column_count++];
        copy_text(column->name, sizeof(column->name), column_name);
        column->type = type;
        if (accept_word(parser, "primary")) {
            if (!accept_word(parser, "key")) {
                printf("Semantic error: expected KEY after PRIMARY for column '%s'.\n", column_name);
                return 0;
            }
            if (candidate.column_count > 1) {
                size_t previous;
                for (previous = 0; previous + 1 < candidate.column_count; previous++) {
                    if (candidate.columns[previous].is_primary_key) {
                        printf("Semantic error: table '%s' can have only one primary key.\n",
                               candidate.name);
                        return 0;
                    }
                }
            }
            column->is_primary_key = 1;
        }

        if (accept_symbol(parser, ",")) {
            continue;
        }
        break;
    } while (1);

    if (!accept_symbol(parser, ")")) {
        printf("Semantic error: expected ')' after column definitions in table '%s'.\n",
               candidate.name);
        return 0;
    }
    if (!expect_end(parser)) {
        return 0;
    }

    database->tables[database->table_count++] = candidate;
    return 1;
}

static int parse_create(SemanticParser *parser)
{
    if (!accept_word(parser, "create")) {
        return 0;
    }
    if (accept_word(parser, "database")) {
        return parse_create_database(parser);
    }
    if (accept_word(parser, "table")) {
        return parse_create_table(parser);
    }
    printf("Semantic error: CREATE must be followed by DATABASE or TABLE.\n");
    return 0;
}

static int resolve_column(SemanticDatabase *database, SemanticTable **scope,
                          size_t scope_count, const char *name,
                          SemanticTable **owner, int *column_index)
{
    size_t table_index;
    SemanticTable *match_table = NULL;
    int match_column = -1;
    int matches = 0;

    for (table_index = 0; table_index < scope_count; table_index++) {
        SemanticTable *table = scope[table_index];
        int found = find_column_index(table, name);
        (void)database;
        if (found >= 0) {
            match_table = table;
            match_column = found;
            matches++;
        }
    }
    if (matches == 1) {
        *owner = match_table;
        *column_index = match_column;
        return 1;
    }
    if (matches > 1) {
        printf("Semantic error: column '%s' is ambiguous across the selected tables.\n", name);
    } else {
        printf("Semantic error: column '%s' does not exist in the selected tables.\n", name);
    }
    return 0;
}

static int value_type_for_operand(SemanticDatabase *database, SemanticTable **scope,
                                 size_t scope_count, const SemanticToken *token,
                                 ValueType *type, SemanticTable **owner, int *column_index)
{
    if (token->type == TOK_STRING) {
        *type = VALUE_TEXT;
        *owner = NULL;
        *column_index = -1;
        return 1;
    }
    if (token->type == TOK_NUMBER) {
        *type = strchr(token->text, '.') == NULL ? VALUE_INTEGER : VALUE_FLOAT;
        *owner = NULL;
        *column_index = -1;
        return 1;
    }
    if (token->type == TOK_WORD &&
        resolve_column(database, scope, scope_count, token->text, owner, column_index)) {
        *type = (*owner)->columns[*column_index].type;
        return 1;
    }
    if (token->type != TOK_WORD) {
        printf("Semantic error: expected a column or literal near '%s'.\n", token->text);
    }
    return 0;
}

static int types_compatible(ValueType left, ValueType right)
{
    return left == right || (left != VALUE_TEXT && right != VALUE_TEXT);
}

static int validate_where(SemanticParser *parser, SemanticDatabase *database,
                          SemanticTable **scope, size_t scope_count,
                          size_t *where_start)
{
    *where_start = parser->position;
    if (!accept_word(parser, "where")) {
        return 1;
    }
    *where_start = parser->position;
    while (parser->position < parser->count) {
        SemanticToken *left = current_token(parser);
        ValueType left_type;
        SemanticTable *left_owner;
        int left_column;
        SemanticToken *operator_token;
        SemanticToken *right;
        ValueType right_type;
        SemanticTable *right_owner;
        int right_column;

        if (!value_type_for_operand(database, scope, scope_count, left,
                                    &left_type, &left_owner, &left_column)) {
            return 0;
        }
        parser->position++;
        operator_token = current_token(parser);
        if (operator_token->type != TOK_SYMBOL ||
            (strcmp(operator_token->text, "=") != 0 &&
             strcmp(operator_token->text, "!=") != 0 &&
             strcmp(operator_token->text, ">") != 0 &&
             strcmp(operator_token->text, "<") != 0 &&
             strcmp(operator_token->text, ">=") != 0 &&
             strcmp(operator_token->text, "<=") != 0)) {
            printf("Semantic error: expected a comparison operator near '%s'.\n",
                   operator_token->text);
            return 0;
        }
        parser->position++;
        right = current_token(parser);
        if (!value_type_for_operand(database, scope, scope_count, right,
                                    &right_type, &right_owner, &right_column)) {
            return 0;
        }
        if (!types_compatible(left_type, right_type)) {
            printf("Semantic error: cannot compare %s with %s.\n",
                   type_name(left_type), type_name(right_type));
            return 0;
        }
        if ((strcmp(operator_token->text, ">") == 0 ||
             strcmp(operator_token->text, "<") == 0 ||
             strcmp(operator_token->text, ">=") == 0 ||
             strcmp(operator_token->text, "<=") == 0) &&
            left_type == VALUE_TEXT && right_type != VALUE_TEXT) {
            printf("Semantic error: ordered comparisons require operands of compatible types.\n");
            return 0;
        }
        parser->position++;
        if (parser->position == parser->count) {
            break;
        }
        if (!accept_word(parser, "and") && !accept_word(parser, "or")) {
            printf("Semantic error: expected AND, OR, or end of WHERE clause near '%s'.\n",
                   current_token(parser)->text);
            return 0;
        }
        if (parser->position == parser->count) {
            printf("Semantic error: WHERE clause ends with a logical operator.\n");
            return 0;
        }
    }
    return 1;
}

static int analyze_select(SemanticParser *parser, SemanticDatabase *database,
                          SemanticColumn *result_columns, size_t *result_count,
                          int allow_trailing_tokens)
{
    typedef struct {
        int aggregate;
        int star_argument;
        char name[SEM_TOKEN_LENGTH];
        char argument[SEM_TOKEN_LENGTH];
    } SelectItem;
    SelectItem items[SEM_MAX_COLUMNS];
    SemanticTable *scope[SEM_MAX_TABLES];
    size_t scope_count = 0;
    int select_all = 0;
    int aggregate_seen = 0;
    int plain_column_seen = 0;
    size_t selected_count = 0;
    size_t item_index;

    if (!accept_word(parser, "select")) {
        printf("Semantic error: expected SELECT.\n");
        return 0;
    }

    memset(items, 0, sizeof(items));
    do {
        SemanticToken *token = current_token(parser);
        if (accept_symbol(parser, "*")) {
            if (selected_count != 0) {
                printf("Semantic error: '*' cannot be combined with other selected columns.\n");
                return 0;
            }
            select_all = 1;
            selected_count++;
        } else if (token->type == TOK_WORD) {
            SelectItem *item;
            if (selected_count == SEM_MAX_COLUMNS) {
                printf("Semantic error: query returns too many columns.\n");
                return 0;
            }
            item = &items[selected_count];
            copy_text(item->name, sizeof(item->name), token->text);
            parser->position++;
            if (accept_symbol(parser, "(")) {
                SemanticToken *argument = current_token(parser);
                item->aggregate = 1;
                if (!text_equal(item->name, "count") && !text_equal(item->name, "min") &&
                    !text_equal(item->name, "max") && !text_equal(item->name, "average")) {
                    printf("Semantic error: unsupported aggregate function '%s'.\n", item->name);
                    return 0;
                }
                if (accept_symbol(parser, "*")) {
                    if (!text_equal(item->name, "count")) {
                        printf("Semantic error: only COUNT accepts '*'.\n");
                        return 0;
                    }
                    item->star_argument = 1;
                } else {
                    if (argument->type != TOK_WORD) {
                        printf("Semantic error: aggregate '%s' requires a column name.\n",
                               item->name);
                        return 0;
                    }
                    copy_text(item->argument, sizeof(item->argument), argument->text);
                    parser->position++;
                }
                if (!accept_symbol(parser, ")")) {
                    printf("Semantic error: expected ')' after aggregate argument.\n");
                    return 0;
                }
                aggregate_seen = 1;
            } else {
                copy_text(item->argument, sizeof(item->argument), item->name);
                plain_column_seen = 1;
            }
            selected_count++;
        } else {
            printf("Semantic error: expected a selected column or aggregate near '%s'.\n",
                   token->text);
            return 0;
        }
    } while (accept_symbol(parser, ","));

    if (!accept_word(parser, "from")) {
        printf("Semantic error: SELECT requires a FROM clause.\n");
        return 0;
    }
    do {
        char table_name[SEM_TOKEN_LENGTH];
        SemanticTable *table;
        if (!expect_word(parser, table_name, sizeof(table_name), "a table name after FROM")) {
            return 0;
        }
        table = find_table(database, table_name);
        if (table == NULL) {
            printf("Semantic error: table '%s' does not exist in database '%s'.\n",
                   table_name, database->name);
            return 0;
        }
        {
            size_t previous;
            for (previous = 0; previous < scope_count; previous++) {
                if (scope[previous] == table) {
                    printf("Semantic error: table '%s' appears more than once in FROM.\n",
                           table_name);
                    return 0;
                }
            }
        }
        if (scope_count == SEM_MAX_TABLES) {
            printf("Semantic error: SELECT references too many tables.\n");
            return 0;
        }
        scope[scope_count++] = table;
    } while (accept_symbol(parser, ","));

    if (select_all) {
        if (result_columns != NULL) {
            size_t table_index;
            selected_count = 0;
            for (table_index = 0; table_index < scope_count; table_index++) {
                size_t column;
                for (column = 0; column < scope[table_index]->column_count; column++) {
                    size_t previous;
                    if (selected_count == SEM_MAX_COLUMNS) {
                        printf("Semantic error: query returns too many columns.\n");
                        return 0;
                    }
                    for (previous = 0; previous < selected_count; previous++) {
                        if (text_equal(result_columns[previous].name,
                                       scope[table_index]->columns[column].name)) {
                            printf("Semantic error: SELECT * produces duplicate column '%s' for LET.\n",
                                   scope[table_index]->columns[column].name);
                            return 0;
                        }
                    }
                    result_columns[selected_count++] = scope[table_index]->columns[column];
                }
            }
        }
    } else {
        for (item_index = 0; item_index < selected_count; item_index++) {
            SemanticTable *owner = NULL;
            int column_index = -1;
            ValueType item_type;
            if (items[item_index].aggregate && items[item_index].star_argument) {
                item_type = VALUE_INTEGER;
            } else {
                const char *column_name = items[item_index].aggregate ?
                                          items[item_index].argument : items[item_index].name;
                if (!resolve_column(database, scope, scope_count, column_name,
                                    &owner, &column_index)) {
                    return 0;
                }
                item_type = owner->columns[column_index].type;
                if (items[item_index].aggregate &&
                    text_equal(items[item_index].name, "average") &&
                    item_type == VALUE_TEXT) {
                    printf("Semantic error: AVERAGE requires a numeric column.\n");
                    return 0;
                }
            }
            if (result_columns != NULL) {
                copy_text(result_columns[item_index].name,
                          sizeof(result_columns[item_index].name), items[item_index].name);
                result_columns[item_index].type =
                    items[item_index].aggregate &&
                    text_equal(items[item_index].name, "average") ? VALUE_FLOAT : item_type;
            }
        }
    }

    if (aggregate_seen && plain_column_seen) {
        printf("Semantic error: aggregate expressions cannot be mixed with plain columns.\n");
        return 0;
    }
    {
        size_t where_start;
        if (is_word(parser, "where")) {
            if (!validate_where(parser, database, scope, scope_count, &where_start)) {
                return 0;
            }
        }
    }
    if (!allow_trailing_tokens && !expect_end(parser)) {
        return 0;
    }
    if (result_count != NULL) {
        *result_count = selected_count;
    }
    return 1;
}

static int parse_select_statement(SemanticParser *parser)
{
    return analyze_select(parser, active_db(), NULL, NULL, 0);
}

static int parse_insert(SemanticParser *parser)
{
    SemanticDatabase *database = active_db();
    SemanticTable *table;
    SemanticToken values[SEM_MAX_COLUMNS];
    char table_name[SEM_TOKEN_LENGTH];
    size_t count = 0;
    size_t column;

    if (!accept_word(parser, "insert") ||
        !expect_word(parser, table_name, sizeof(table_name), "a target table name")) {
        return 0;
    }
    table = find_table(database, table_name);
    if (table == NULL) {
        printf("Semantic error: table '%s' does not exist in database '%s'.\n",
               table_name, database->name);
        return 0;
    }
    if (!accept_word(parser, "values") || !accept_symbol(parser, "(")) {
        printf("Semantic error: INSERT requires VALUES followed by '('.\n");
        return 0;
    }
    if (!accept_symbol(parser, ")")) {
        do {
            SemanticToken *token = current_token(parser);
            if (count == SEM_MAX_COLUMNS) {
                printf("Semantic error: INSERT supplies too many values.\n");
                return 0;
            }
            if (token->type != TOK_STRING && token->type != TOK_NUMBER) {
                printf("Semantic error: INSERT values must be string or numeric literals (near '%s').\n",
                       token->text);
                return 0;
            }
            values[count++] = *token;
            parser->position++;
        } while (accept_symbol(parser, ","));
        if (!accept_symbol(parser, ")")) {
            printf("Semantic error: expected ')' after INSERT values.\n");
            return 0;
        }
    }
    if (!expect_end(parser)) {
        return 0;
    }
    if (count != table->column_count) {
        printf("Semantic error: table '%s' expects %zu values, but INSERT supplies %zu.\n",
               table_name, table->column_count, count);
        return 0;
    }
    for (column = 0; column < count; column++) {
        if (!token_type_matches(&values[column], table->columns[column].type)) {
            printf("Semantic error: value for column '%s' must have type %s.\n",
                   table->columns[column].name, type_name(table->columns[column].type));
            return 0;
        }
        if (table->columns[column].is_primary_key &&
            duplicate_primary_key(table, values[column].text, (size_t)-1)) {
            printf("Semantic error: duplicate primary key '%s' for table '%s'.\n",
                   values[column].text, table_name);
            return 0;
        }
    }
    return append_row(table, values, count);
}

static int compare_values(const char *left, const char *right, ValueType type)
{
    if (type == VALUE_TEXT) {
        while (*left != '\0' && *right != '\0') {
            int left_character = tolower((unsigned char)*left);
            int right_character = tolower((unsigned char)*right);
            if (left_character != right_character) {
                return left_character < right_character ? -1 : 1;
            }
            left++;
            right++;
        }
        return *left == *right ? 0 : (*left == '\0' ? -1 : 1);
    }
    {
        double left_value = strtod(left, NULL);
        double right_value = strtod(right, NULL);
        return left_value < right_value ? -1 : left_value > right_value ? 1 : 0;
    }
}

static int evaluate_condition(const SemanticToken *tokens, size_t start, size_t end,
                              SemanticTable *table, const SemanticRow *row)
{
    int result = 1;
    int pending_and = 1;
    size_t index = start;

    while (index + 2 < end) {
        int left_column = find_column_index(table, tokens[index].text);
        int right_column = find_column_index(table, tokens[index + 2].text);
        const char *left_value;
        const char *right_value;
        ValueType left_type;
        int comparison;
        int condition;
        const char *operator_text = tokens[index + 1].text;

        if (left_column >= 0 && tokens[index].type == TOK_WORD) {
            left_value = row->values[left_column];
            left_type = table->columns[left_column].type;
        } else {
            left_value = tokens[index].text;
            left_type = tokens[index].type == TOK_STRING ? VALUE_TEXT :
                        (strchr(tokens[index].text, '.') == NULL ? VALUE_INTEGER : VALUE_FLOAT);
        }
        if (right_column >= 0 && tokens[index + 2].type == TOK_WORD) {
            right_value = row->values[right_column];
        } else {
            right_value = tokens[index + 2].text;
        }
        comparison = compare_values(left_value, right_value, left_type);
        if (strcmp(operator_text, "=") == 0) condition = comparison == 0;
        else if (strcmp(operator_text, "!=") == 0) condition = comparison != 0;
        else if (strcmp(operator_text, ">") == 0) condition = comparison > 0;
        else if (strcmp(operator_text, "<") == 0) condition = comparison < 0;
        else if (strcmp(operator_text, ">=") == 0) condition = comparison >= 0;
        else condition = comparison <= 0;

        if (pending_and) {
            result = result && condition;
        } else {
            result = result || condition;
        }
        index += 3;
        if (index < end) {
            pending_and = text_equal(tokens[index].text, "and");
            index++;
        }
    }
    return result;
}

static int row_matches_where(const SemanticToken *tokens, size_t start, size_t end,
                             SemanticTable *table, const SemanticRow *row)
{
    size_t index;
    int group_result = 1;
    int expression_result = 0;
    size_t group_start = start;

    for (index = start; index <= end; index++) {
        if (index == end || text_equal(tokens[index].text, "or")) {
            group_result = evaluate_condition(tokens, group_start, index, table, row);
            expression_result = expression_result || group_result;
            group_start = index + 1;
        }
    }
    return expression_result;
}

static int parse_update(SemanticParser *parser)
{
    SemanticDatabase *database = active_db();
    SemanticTable *table;
    char table_name[SEM_TOKEN_LENGTH];
    size_t update_columns[SEM_MAX_COLUMNS];
    SemanticToken update_values[SEM_MAX_COLUMNS];
    size_t update_count = 0;
    size_t where_start;
    size_t where_end;
    size_t assignment;
    size_t row;
    int *matched_rows = NULL;
    char ***replacement_values = NULL;
    int key_column = -1;

    if (!accept_word(parser, "update") ||
        !expect_word(parser, table_name, sizeof(table_name), "a target table name")) {
        return 0;
    }
    table = find_table(database, table_name);
    if (table == NULL) {
        printf("Semantic error: table '%s' does not exist in database '%s'.\n",
               table_name, database->name);
        return 0;
    }
    if (!accept_word(parser, "set")) {
        printf("Semantic error: UPDATE requires SET.\n");
        return 0;
    }
    do {
        char column_name[SEM_TOKEN_LENGTH];
        int column_index;
        SemanticToken *value;
        if (!expect_word(parser, column_name, sizeof(column_name), "an updated column")) {
            return 0;
        }
        column_index = find_column_index(table, column_name);
        if (column_index < 0) {
            printf("Semantic error: column '%s' does not exist in table '%s'.\n",
                   column_name, table_name);
            return 0;
        }
        for (assignment = 0; assignment < update_count; assignment++) {
            if (update_columns[assignment] == (size_t)column_index) {
                printf("Semantic error: column '%s' is assigned more than once.\n", column_name);
                return 0;
            }
        }
        if (!accept_symbol(parser, "=")) {
            printf("Semantic error: expected '=' after updated column '%s'.\n", column_name);
            return 0;
        }
        value = current_token(parser);
        if (value->type != TOK_STRING && value->type != TOK_NUMBER) {
            printf("Semantic error: UPDATE value for '%s' must be a literal.\n", column_name);
            return 0;
        }
        if (!token_type_matches(value, table->columns[column_index].type)) {
            printf("Semantic error: value for column '%s' must have type %s.\n",
                   column_name, type_name(table->columns[column_index].type));
            return 0;
        }
        if (update_count == SEM_MAX_COLUMNS) {
            printf("Semantic error: UPDATE has too many assignments.\n");
            return 0;
        }
        update_columns[update_count] = (size_t)column_index;
        update_values[update_count++] = *value;
        parser->position++;
    } while (accept_symbol(parser, ","));

    where_start = parser->position;
    if (is_word(parser, "where")) {
        if (!validate_where(parser, database, (SemanticTable *[]){ table }, 1, &where_start)) {
            return 0;
        }
    }
    where_end = parser->count;
    if (!expect_end(parser)) {
        return 0;
    }

    if (table->row_count > 0) {
        matched_rows = calloc(table->row_count, sizeof(*matched_rows));
        replacement_values = calloc(table->row_count, sizeof(*replacement_values));
        if (matched_rows == NULL || replacement_values == NULL) {
            free(matched_rows);
            free(replacement_values);
            printf("Semantic error: unable to allocate UPDATE workspace.\n");
            return 0;
        }
    }
    for (assignment = 0; assignment < update_count; assignment++) {
        if (table->columns[update_columns[assignment]].is_primary_key) {
            key_column = (int)update_columns[assignment];
        }
    }
    for (row = 0; row < table->row_count; row++) {
        matched_rows[row] = where_start == parser->count ||
            row_matches_where(parser->tokens, where_start, where_end, table,
                              &table->rows[row]);
    }
    if (key_column >= 0) {
        size_t other_row;
        ValueType key_type = table->columns[key_column].type;
        SemanticToken *new_key = NULL;
        for (assignment = 0; assignment < update_count; assignment++) {
            if (update_columns[assignment] == (size_t)key_column) {
                new_key = &update_values[assignment];
                break;
            }
        }
        if (new_key != NULL) {
            for (row = 0; row < table->row_count; row++) {
                for (other_row = row + 1; other_row < table->row_count; other_row++) {
                    const char *row_key = matched_rows[row] ? new_key->text :
                        table->rows[row].values[key_column];
                    const char *other_key = matched_rows[other_row] ? new_key->text :
                        table->rows[other_row].values[key_column];
                    if (values_equal(row_key, other_key, key_type)) {
                        free(matched_rows);
                        free(replacement_values);
                        printf("Semantic error: UPDATE would create a duplicate primary key in table '%s'.\n",
                               table_name);
                        return 0;
                    }
                }
            }
        }
    }

    for (row = 0; row < table->row_count; row++) {
        if (matched_rows[row]) {
            replacement_values[row] = calloc(update_count, sizeof(*replacement_values[row]));
            if (replacement_values[row] == NULL) {
                size_t cleanup_row;
                for (cleanup_row = 0; cleanup_row < row; cleanup_row++) {
                    size_t cleanup_assignment;
                    for (cleanup_assignment = 0; replacement_values[cleanup_row] != NULL &&
                         cleanup_assignment < update_count; cleanup_assignment++) {
                        free(replacement_values[cleanup_row][cleanup_assignment]);
                    }
                    free(replacement_values[cleanup_row]);
                }
                free(matched_rows);
                free(replacement_values);
                printf("Semantic error: unable to allocate updated values.\n");
                return 0;
            }
            for (assignment = 0; assignment < update_count; assignment++) {
                replacement_values[row][assignment] =
                    malloc(strlen(update_values[assignment].text) + 1);
                if (replacement_values[row][assignment] == NULL) {
                    size_t cleanup_row;
                    size_t cleanup_assignment;
                    for (cleanup_row = 0; cleanup_row <= row; cleanup_row++) {
                        for (cleanup_assignment = 0;
                             replacement_values[cleanup_row] != NULL &&
                             cleanup_assignment < update_count; cleanup_assignment++) {
                            free(replacement_values[cleanup_row][cleanup_assignment]);
                        }
                        free(replacement_values[cleanup_row]);
                    }
                    free(matched_rows);
                    free(replacement_values);
                    printf("Semantic error: unable to allocate updated value.\n");
                    return 0;
                }
                strcpy(replacement_values[row][assignment], update_values[assignment].text);
            }
        }
    }

    for (row = 0; row < table->row_count; row++) {
        if (matched_rows[row]) {
            for (assignment = 0; assignment < update_count; assignment++) {
                size_t column = update_columns[assignment];
                free(table->rows[row].values[column]);
                table->rows[row].values[column] = replacement_values[row][assignment];
            }
            free(replacement_values[row]);
        }
    }
    free(matched_rows);
    free(replacement_values);
    return 1;
}

static int parse_delete(SemanticParser *parser)
{
    SemanticDatabase *database = active_db();
    SemanticTable *table;
    char table_name[SEM_TOKEN_LENGTH];
    size_t where_start;
    size_t row;

    if (!accept_word(parser, "delete")) {
        return 0;
    }
    accept_word(parser, "from");
    if (!expect_word(parser, table_name, sizeof(table_name), "a table name after DELETE")) {
        return 0;
    }
    table = find_table(database, table_name);
    if (table == NULL) {
        printf("Semantic error: table '%s' does not exist in database '%s'.\n",
               table_name, database->name);
        return 0;
    }
    if (parser->position == parser->count) {
        int table_index = find_table_index(database, table_name);
        free_table_rows(table);
        if ((size_t)table_index + 1 < database->table_count) {
            memmove(&database->tables[table_index], &database->tables[table_index + 1],
                    (database->table_count - (size_t)table_index - 1) *
                    sizeof(database->tables[0]));
        }
        database->table_count--;
        memset(&database->tables[database->table_count], 0, sizeof(database->tables[0]));
        return 1;
    }
    if (!is_word(parser, "where")) {
        printf("Semantic error: DELETE accepts only an optional WHERE clause after the table name.\n");
        return 0;
    }
    where_start = parser->position;
    {
        size_t ignored_start;
        if (!validate_where(parser, database, (SemanticTable *[]){ table }, 1, &ignored_start)) {
            return 0;
        }
        where_start = ignored_start;
    }
    if (!expect_end(parser)) {
        return 0;
    }
    for (row = 0; row < table->row_count;) {
        if (row_matches_where(parser->tokens, where_start, parser->count, table,
                              &table->rows[row])) {
            size_t column;
            for (column = 0; column < table->column_count; column++) {
                free(table->rows[row].values[column]);
            }
            free(table->rows[row].values);
            if (row + 1 < table->row_count) {
                memmove(&table->rows[row], &table->rows[row + 1],
                        (table->row_count - row - 1) * sizeof(table->rows[0]));
            }
            table->row_count--;
        } else {
            row++;
        }
    }
    return 1;
}

static int parse_rename(SemanticParser *parser)
{
    SemanticTable *table;
    char table_name[SEM_TOKEN_LENGTH];
    char new_names[SEM_MAX_COLUMNS][SEM_TOKEN_LENGTH];
    size_t count = 0;
    size_t left;

    if (!accept_word(parser, "rename") ||
        !expect_word(parser, table_name, sizeof(table_name), "a table name after RENAME")) {
        return 0;
    }
    table = find_table(active_db(), table_name);
    if (table == NULL) {
        printf("Semantic error: table '%s' does not exist in database '%s'.\n",
               table_name, active_db()->name);
        return 0;
    }
    if (!accept_symbol(parser, "(")) {
        printf("Semantic error: RENAME requires a parenthesized list of new column names.\n");
        return 0;
    }
    do {
        if (count == SEM_MAX_COLUMNS ||
            !expect_word(parser, new_names[count], sizeof(new_names[count]), "a new column name")) {
            return 0;
        }
        count++;
    } while (accept_symbol(parser, ","));

    if (!accept_symbol(parser, ")") || !expect_end(parser)) {
        return 0;
    }
    if (count != table->column_count) {
        printf("Semantic error: RENAME for table '%s' requires %zu column names, but received %zu.\n",
               table_name, table->column_count, count);
        return 0;
    }
    for (left = 0; left < count; left++) {
        size_t right;
        for (right = left + 1; right < count; right++) {
            if (text_equal(new_names[left], new_names[right])) {
                printf("Semantic error: renamed column '%s' would be duplicated.\n", new_names[left]);
                return 0;
            }
        }
    }
    for (left = 0; left < count; left++) {
        copy_text(table->columns[left].name, sizeof(table->columns[left].name), new_names[left]);
    }
    return 1;
}

static int parse_let(SemanticParser *parser)
{
    SemanticDatabase *database = active_db();
    SemanticColumn result_columns[SEM_MAX_COLUMNS];
    size_t result_count = 0;
    char result_name[SEM_TOKEN_LENGTH];
    char key_name[SEM_TOKEN_LENGTH];
    size_t column;
    SemanticTable result;

    if (!accept_word(parser, "let") ||
        !expect_word(parser, result_name, sizeof(result_name), "a LET result name") ||
        !accept_word(parser, "key") ||
        !expect_word(parser, key_name, sizeof(key_name), "a key column after KEY")) {
        if (parser->position < parser->count) {
            printf("Semantic error: LET syntax is LET name KEY column SELECT ... .\n");
        }
        return 0;
    }
    if (find_table(database, result_name) != NULL) {
        printf("Semantic error: table '%s' already exists in database '%s'.\n",
               result_name, database->name);
        return 0;
    }
    if (!analyze_select(parser, database, result_columns, &result_count, 0)) {
        return 0;
    }
    memset(&result, 0, sizeof(result));
    copy_text(result.name, sizeof(result.name), result_name);
    result.column_count = result_count;
    for (column = 0; column < result_count; column++) {
        result.columns[column] = result_columns[column];
        if (text_equal(result.columns[column].name, key_name)) {
            result.columns[column].is_primary_key = 1;
        }
    }
    if (result_count == 0 || find_column_index(&result, key_name) < 0) {
        printf("Semantic error: LET key column '%s' must appear in the SELECT list.\n", key_name);
        return 0;
    }
    if (database->table_count == SEM_MAX_TABLES) {
        printf("Semantic error: table catalog is full for database '%s'.\n", database->name);
        return 0;
    }
    database->tables[database->table_count++] = result;
    return 1;
}

int semantic_analyze_statement(const char *statement)
{
    SemanticParser parser;
    SemanticToken *first;
    int result;

    if (statement == NULL) {
        printf("Semantic error: statement is NULL.\n");
        return 0;
    }
    ensure_initialized();
    if (!tokenize_statement(statement, &parser)) {
        return 0;
    }
    if (parser.count == 0) {
        return 1;
    }
    first = current_token(&parser);
    if (first->type == TOK_SYMBOL && strcmp(first->text, ";") == 0) {
        return 1;
    }
    if (first->type == TOK_SYMBOL && strcmp(first->text, "-") == 0) {
        return 1;
    }

    if (accept_word(&parser, "create")) {
        parser.position = 0;
        result = parse_create(&parser);
    } else if (accept_word(&parser, "use")) {
        char name[SEM_TOKEN_LENGTH];
        parser.position = 1;
        result = expect_word(&parser, name, sizeof(name), "a database name") &&
                 expect_end(&parser);
        if (result) {
            int index = find_database(name);
            if (index < 0) {
                printf("Semantic error: database '%s' does not exist.\n", name);
                result = 0;
            } else {
                active_database = (size_t)index;
            }
        }
    } else if (accept_word(&parser, "describe") || accept_word(&parser, "desc")) {
        char name[SEM_TOKEN_LENGTH];
        parser.position = 1;
        result = expect_word(&parser, name, sizeof(name), "a table name after DESCRIBE") &&
                 expect_end(&parser);
        if (result && !text_equal(name, "all") && find_table(active_db(), name) == NULL) {
            printf("Semantic error: table '%s' does not exist in database '%s'.\n",
                   name, active_db()->name);
            result = 0;
        }
    } else if (accept_word(&parser, "select")) {
        parser.position = 0;
        result = parse_select_statement(&parser);
    } else if (accept_word(&parser, "insert")) {
        parser.position = 0;
        result = parse_insert(&parser);
    } else if (accept_word(&parser, "update")) {
        parser.position = 0;
        result = parse_update(&parser);
    } else if (accept_word(&parser, "delete")) {
        parser.position = 0;
        result = parse_delete(&parser);
    } else if (accept_word(&parser, "rename")) {
        parser.position = 0;
        result = parse_rename(&parser);
    } else if (accept_word(&parser, "let")) {
        parser.position = 0;
        result = parse_let(&parser);
    } else if (accept_word(&parser, "exit") || accept_word(&parser, "input")) {
        result = 1;
    } else {
        printf("Semantic error: unsupported statement near '%s'.\n", first->text);
        result = 0;
    }
    return result;
}