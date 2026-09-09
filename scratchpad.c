#include <stdio.h>
#include <string.h>

int main() {
    char str[] = "Learn-C-Programming";
    char c = '-'; // The delimiter character
    
    // strtok expects a string of delimiters, so we pass &c or a string literal like "-"
    char *token = strtok(str, "-");
    
    while (token != NULL) {
        printf("%s\n", token);
        token = strtok(NULL, "-");
    }
    return 0;
}
