#ifndef DATABASE_PROJECT_H
#define DATABASE_PROJECT_H

#include "globals.h"

void statement_separator(char separated_statements[][MAX_BUFFER], int *num_statements);
int statement_identifier(char *statement);

#endif
