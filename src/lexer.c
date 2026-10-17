#include "lexer.h"

#include <stdio.h>

// This function gets the character a set distance away from the current character
// Returns a null terminator (\0) if there isn't anything that far ahead
// Runs in O(distance); effectively O(1) because callers pass at most 2
static char lookahead(Lexer* lexer, size_t distance) {
    const char* c = lexer->c;
    size_t i = 0;
    while (*c != '\0') {
        // If c has gone the distance without reaching a null terminator, return it
        if (i == distance) {
            return *c;
        }
        c++; i++;
    }
    // Only return \0 if c reached a null terminator
    return '\0';
}

// Advances the lexer's index by one; \r is considered an ordinary column and only \n ends a line
// nextToken skips \r as whitespace
static void advance(Lexer* lexer) {
    // Consumes the character at c; with a newline starting a new row, 
    // and anything else advances the index; nothing happens at the terminator
    if (*(lexer->c) == '\0') { return; }
    // Newline - go back to the start and one row down
    else if (*(lexer->c) == '\n') {
        lexer->c++;
        lexer->row++;
        lexer->col = 1;
    } else {
        lexer->c++; lexer->col++;
    }
}

// Create a lexer based on the source file, returning a lexer initialized to the first character
// Runs in O(1) time
Lexer createLexer(const char* src) {
    // NULL source is a caller bug, so exit
    if (src == NULL) {
        fprintf(stderr, "Failed to compile source. File is NULL.\n");
        exit(EXIT_FAILURE);
    }
    Lexer l = { .c = src, .col = 1, .row = 1 };
    return l;
}

// Get and return the next token currently in the lexer's source
Token nextToken(Lexer* lexer) {
    const char* start;
    size_t col;
    size_t row;

    // Skip whitespace before tokenizing
    while (*(lexer->c) == ' ' || *(lexer->c) == '\r' || *(lexer->c) == '\t') {
        advance(lexer);
    }

    start = lexer->c;
    col = lexer->col;
    row = lexer->row;

    // Preinitialized invalid token
    Token t = { 
        .kind = INVALID, 
        .typeValue.n = NULL,
        .text = start, 
        .length = 1, 
        .col = col, 
        .row = row 
    };

    // End of file, return base Token with NON_SOURCE kind
    if (*start == '\0') {
        t.kind = NON_SOURCE;
        t.length = 0;
    // This is a newline, return a NEWLINE token
    } else if (*start == '\n') {
        t.kind = NEWLINE;
        t.length = 1;
        advance(lexer);
    // Otherwise, just advance then return the base invalid token
    } else {
        advance(lexer);
    }
    
    return t;
}
