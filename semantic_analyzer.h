#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

/* Reset the in-memory catalog for a new input session. */
void semantic_analyzer_reset(void);

/*
 * Analyze one statement against the in-memory catalog.
 * Returns nonzero when the statement is semantically valid.
 */
int semantic_analyze_statement(const char *statement);

#endif