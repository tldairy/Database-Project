#include <stdio.h>
#include <ctype.h>
#include <string.h>



void keyword_parser(char query[]){
    // char keyword[10];

    size_t length = strcspn(query, " "); 
    
    // Insert the null terminator at that position
    query[length] = '\0'; 
    
    printf("Keyword: %s\n", query);

    // if (toupper(keyword) == "CREATE") {
    //     input_CREATE(query);
    // } else if (toupper(keyword) == "USE") {
    //     input_USE(query);
    // } else if (toupper(keyword) == "DESCRIBE" || toupper(keyword) == "DESC") {
    //     input_DESCRIBE(query);
    // } else if (toupper(keyword) == "SELECT") {
    //     input_SELECT(query);
    // } else if (toupper(keyword) == "LET") {
    //     input_LET(query);
    // } else if (toupper(keyword) == "INSERT") {
    //     input_INSERT(query);
    // } else if (toupper(keyword) == "UPDATE") {
    //     input_UPDATE(query);
    // } else if (toupper(keyword) == "DELETE") {
    //     input_DELETE(query);
    // } else if (toupper(keyword) == "RENAME") {
    //     input_RENAME(query);
    // } else if (toupper(keyword) == "INPUT") {
    //     input_INPUT(query);
    // } else if (toupper(keyword) == "EXIT") {
    //     input_EXIT(query);
    // }
    // else {
    //     printf("Unknown command: %s\n", keyword);
    // }

}

// void input_CREATE(char query[]) {
//     // Implementation for CREATE command
//     char DBname[] = strtok(query, " ");
//     if (DBname == NULL) {
//         printf("Error: Database name is missing.\n");
//         return;
//     }
// }

// void input_USE(char query[]) {
//     // Implementation for USE command
// }

// void input_DESCRIBE(char query[]) {
//     // Implementation for DESCRIBE command
// }

// void input_SELECT(char query[]) {
//     // Implementation for SELECT command
// }

// void input_LET(char query[]) {
//     // Implementation for LET command
// }

// void input_INSERT(char query[]) {
//     // Implementation for INSERT command
// }

// void input_UPDATE(char query[]) {
//     // Implementation for UPDATE command
// }

// void input_DELETE(char query[]) {
//     // Implementation for DELETE command
// }

// void input_RENAME(char query[]) {
//     // Implementation for RENAME command
// }

// void input_INPUT(char query[]) {
//     // Implementation for INPUT command
// }

// void input_EXIT(char query[]) {
//     // Implementation for EXIT command
// }

int main(void)
{
    printf("Database project starter\n");

    char query[256];
    printf("Enter a query: ");
    scanf("%s", query); 
    keyword_parser(query);

    return 0;
}