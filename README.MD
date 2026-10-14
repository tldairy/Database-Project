gcc driver.c globals.c semantic_analyzer.c syntax_analyzer.c -o database-project

Assumption: 
1) SELECT: "min", "max", "count", and "average" reserved keywords, and will throw errors when used as AttrName. 