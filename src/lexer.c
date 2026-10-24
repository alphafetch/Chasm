/**
 * @file lexer.c
 * @brief Lexer functions to lex a given file into Tokens
 */

#include "lexer.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "errors.h"

// Helper macros for character type detection
#define IS_ALPHANUMERIC(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || ((c) >= '0' && (c) <= '9'))
#define IS_ALPHA(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define IS_ALPHANUMERIC_(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || ((c) >= '0' && (c) <= '9') || ((c) == '_'))
#define IS_ALPHA_(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || ((c) == '_'))
#define IS_DOT(c) ((c) == '.')
#define IS_NUMERIC(c) ((c) >= '0' && (c) <= '9')
#define IS_SUFFIX(c) ((c) == 'f' || (c) == 'l' || (c) == 's' || (c) == 'z' || (c) == 'u')

// Must be used safely! Do not use at the beginning of a buffer
#define LOOKBEHIND(lxr, ct) (*(lxr->c - ct))

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
Lexer createLexer(const char* src, const char* filename) {
    // NULL source is a caller bug, so exit
    if (src == NULL) {
        handleExit(NULL_FILE_LEXER_ERROR, EMPTY_STR, NO_ROWCOL_GIVEN, 0, EMPTY_STR);
    }
    Lexer l = { .c = src, .col = 1, .row = 1, .file = filename };
    return l;
}

// Get and return the next token currently in the lexer's source
// This function should be O(n) time per file due to consuming each character once
Token nextToken(Lexer* lexer) {
    const char* start;
    size_t col;
    size_t row;

    start = lexer->c;
    col = lexer->col;
    row = lexer->row;

    // Skip comments and whitespace before tokenizing
    // Loop doesn't stop until skipped tokens are passed, or the end of the file
    while (*(lexer->c) != '\0') {
        // Whitespace - skip
        if (*(lexer->c) == ' ' || *(lexer->c) == '\r' || *(lexer->c) == '\t') {
            advance(lexer);
            
            // Update row and column to match
            col = lexer->col;
            row = lexer->row;
        // Line comment - skip until newline
        } else if (*(lexer->c) == '/' && lookahead(lexer, 1) == '/') {
            while (*(lexer->c) != '\n' && *(lexer->c) != '\0') {
                advance(lexer);
            }
        // Block comment - skip until closed
        } else if (*(lexer->c) == '/' && lookahead(lexer, 1) == '*') {
            // Advance twice to get off of the opener's *
            advance(lexer); advance(lexer);
            while (!(*(lexer->c) == '*' && lookahead(lexer, 1) == '/') && *(lexer->c) != '\0') {
                advance(lexer);
            }

            if (*(lexer->c) == '\0') {
                handleExit(UNTERMINATED_BLOCK_COMMENT_ERROR, lexer->file, row, col, 0, EMPTY_STR);
            }

            // Advance twice to avoid being stuck on the closer's *
            advance(lexer); advance(lexer);
        // No skippable tokens left - break
        } else {
            break;
        }
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
    } 
    
    /**
     * Character table (single character only):
     * Character -> Token Value
     * (         -> PC_LPAREN
     * )         -> PC_RPAREN
     * [         -> PC_LSQUARE
     * ]         -> PC_RSQUARE
     * {         -> PC_LBRACE
     * }         -> PC_RBRACE
     * ,         -> PC_COMMA
     * ;         -> PC_SEMICOLON
     * .         -> PC_DOT
     * :         -> PC_COLON
     * ~         -> PC_BITNOT
     */

    int isPunctuation = 0;
    // Switch the character to see if it is any characters that do not require lookahead
    // All are punctuation, but not all punctuation can be found from the first character, so only these are switched for now
    switch (*start) {
        case '(': t.typeValue.pc = PC_LPAREN;   isPunctuation = 1;   break;
        case ')': t.typeValue.pc = PC_RPAREN;   isPunctuation = 1;   break;
        case '[': t.typeValue.pc = PC_LSQUARE;  isPunctuation = 1;   break;
        case ']': t.typeValue.pc = PC_RSQUARE;  isPunctuation = 1;   break;
        case '{': t.typeValue.pc = PC_LBRACE;   isPunctuation = 1;   break;
        case '}': t.typeValue.pc = PC_RBRACE;   isPunctuation = 1;   break;
        case ',': t.typeValue.pc = PC_COMMA;    isPunctuation = 1;   break;
        case ';': t.typeValue.pc = PC_SEMICOLON;isPunctuation = 1;   break;
        case '.': t.typeValue.pc = PC_DOT;      isPunctuation = 1;   break;
        case ':': t.typeValue.pc = PC_COLON;    isPunctuation = 1;   break;
        case '~': t.typeValue.pc = PC_BITNOT;   isPunctuation = 1;   break;
    }

    /**
     * Character table:
     * First ->  Characters following it  -> Single character version
     * +     ->  + (++), = (+=)           -> +
     * -     ->  - (--), = (-=)           -> -
     * *     ->  = (*=)                   -> *
     * /     ->  = (/=), / (//), * (/ *)  -> /
     * %     ->  = (%=)                   -> %
     * &     ->  & (&&), = (&=)           -> &
     * |     ->  | (||), = (|=)           -> |
     * ^     ->  ^ (^^), = (^=)           -> ^
     * =     ->  = (==), > (=>)           -> =
     * !     ->  = (!=)                   -> !
     * <     ->  = (<=), < (<<), <= (<<=) -> <
     * >     ->  = (>=), > (>>), >= (>>=) -> >
     */

    int isTwoCharPunctuation = 0;
    int isThreeCharPunctuation = 0;

    // Evaluate the above table
    char next = lookahead(lexer, 1);
    char tail = lookahead(lexer, 2);
    switch(*start) {
        case '+':
            if (next == '+') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_INCREMENT;
            } else if (next == '=') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_PLUSASSIGN;
            } else {
                isPunctuation = 1;
                t.typeValue.pc = PC_PLUS;
            }
            break;
        case '-':
            if (next == '-') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_DECREMENT;
            } else if (next == '=') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_MINUSASSIGN;
            } else {
                isPunctuation = 1;
                t.typeValue.pc = PC_MINUS;
            }
            break;
        case '*':
            if (next == '=') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_TIMESASSIGN;
            } else {
                isPunctuation = 1;
                t.typeValue.pc = PC_TIMES;
            }
            break;
        case '/':
            if (next == '=') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_DIVIDEASSIGN;
            } else {
                isPunctuation = 1;
                t.typeValue.pc = PC_DIVIDE;
            }
            break;
        case '%':
            if (next == '=') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_MODASSIGN;
            } else {
                isPunctuation = 1;
                t.typeValue.pc = PC_MOD;
            }
            break;
        case '&':
            if (next == '&') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_AND;
            } else if (next == '=') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_BITANDASSIGN;
            } else {
                isPunctuation = 1;
                t.typeValue.pc = PC_BITAND;
            }
            break;
        case '|':
            if (next == '|') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_OR;
            } else if (next == '=') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_BITORASSIGN;
            } else {
                isPunctuation = 1;
                t.typeValue.pc = PC_BITOR;
            }
            break;
        case '^':
            if (next == '^') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_DIRECTIVE;
            } else if (next == '=') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_BITXORASSIGN;
            } else {
                isPunctuation = 1;
                t.typeValue.pc = PC_BITXOR;
            }
            break;
        case '=':
            if (next == '=') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_EQUAL;
            } else if (next == '>') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_TERNARY;
            } else {
                isPunctuation = 1;
                t.typeValue.pc = PC_ASSIGN;
            }
            break;
        case '!':
            if (next == '=') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_NEQUAL;
            } else {
                isPunctuation = 1;
                t.typeValue.pc = PC_NOT;
            }
            break;
        case '<':
            if (next == '<') {
                if (tail == '=') {
                    isThreeCharPunctuation = 1;
                    t.typeValue.pc = PC_LEFTASSIGN;
                } else {
                    isTwoCharPunctuation = 1;
                    t.typeValue.pc = PC_SHIFTLEFT;
                }
            } else if (next == '=') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_LTEQUAL;
            } else {
                isPunctuation = 1;
                t.typeValue.pc = PC_LT;
            }
            break;
        case '>':
            if (next == '>') {
                if (tail == '=') {
                    isThreeCharPunctuation = 1;
                    t.typeValue.pc = PC_RIGHTASSIGN;
                } else {
                    isTwoCharPunctuation = 1;
                    t.typeValue.pc = PC_SHIFTRIGHT;
                }
            } else if (next == '=') {
                isTwoCharPunctuation = 1;
                t.typeValue.pc = PC_GTEQUAL;
            } else {
                isPunctuation = 1;
                t.typeValue.pc = PC_GT;
            }
            break;
    }

    // Invalid means no type has yet to be found for the token
    if (t.kind == INVALID) {
        // Check if the identifier starts with a-z A-Z or _
        if (IS_ALPHA_((unsigned char)*start)) {
            t.kind = IDENTIFIER;

            // Continue while the identifier is a-z A-Z 0-9 or _
            while (IS_ALPHANUMERIC_((unsigned char)*(lexer->c))) {
                advance(lexer);
            }

            // Set the token's length to the length of what was consumed
            // Need an explicit (size_t) conversion because lexer->c - start returns a ptrdiff_t
            t.length = (size_t)(lexer->c - start);
        }
    }

    // If it's an identifier, there's a chance it is a keyword, so compare to the list defined in keywords.c
    if (t.kind == IDENTIFIER) {
        for (int i = 0; i < KEYWORD_COUNT; i++) {
            if (KEYWORD_LIST[i].length != t.length) {
                continue;
            }
            
            if (memcmp(t.text, KEYWORD_LIST[i].text, t.length) == 0) {
                t.kind = KEYWORD;
                t.typeValue.kw = KEYWORD_LIST[i].kw;
                break;
            }
        }
    }

    /**
     * number   = digits [ '.' digits [ exponent ] ] suffix
     * exponent = 'e' [ '+' | '-' ] digits
     * suffix   = any run of the letters f l s z u without duplicates
     */

    if (t.kind == INVALID && IS_NUMERIC(*start)) {
        // Scanner - Go through the number, taking in anything
        while (IS_ALPHANUMERIC_(*(lexer->c)) || IS_DOT(*(lexer->c))) {
            advance(lexer);
            // LOOKBEHIND safely used, advance is used prior
            if (LOOKBEHIND(lexer, 1) == 'e') {
                if (*(lexer->c) == '+' || *(lexer->c) == '-') {
                    advance(lexer);
                    continue;
                }
            }
        }

        t.length = (size_t)(lexer->c - start);
        t.kind = NUM_LITERAL;

        // Validator - Check the scanned number to verify its validity
        size_t i = 0;
        const char* validatorPtr = start;
        short fraction = 0;
        
        // Phase 1 - Consume digits until there is a dot
        while (IS_NUMERIC(validatorPtr[i])) i++;
        
        // Phase 2 - If the character is a dot, consume it, and any digits after it
        if (IS_DOT(validatorPtr[i])) {
            i++; fraction = 1;
            if (!IS_NUMERIC(validatorPtr[i])) {
                t.kind = INVALID;
            }
        }
        while (IS_NUMERIC(validatorPtr[i])) i++;

        // Phase 3 - If there was a fraction, and the current character is e, consume the exponent
        if (validatorPtr[i] == 'e') {
            if (fraction) {
                i++;
                if ((validatorPtr[i] == '+' || validatorPtr[i] == '-') && IS_NUMERIC(validatorPtr[i + 1])) i++;
                else if (IS_NUMERIC(validatorPtr[i])) i++;
                else t.kind = INVALID;
                while (IS_NUMERIC(validatorPtr[i])) {
                    i++;
                }
            }
        }

        // Phase 4 - Any remaining alpha letters must be suffixes
        while (IS_SUFFIX(validatorPtr[i])) i++;

        // Phase 5 - Check if it reached the end
        if (i != t.length) t.kind = INVALID;
    }

    // Check what type of punctuation it is, if it is a punctuation token, and apply necessary changes
    if (isThreeCharPunctuation) {
        t.kind = PUNCTUATION;
        t.length = 3;
        // Advance the lexer thrice for three-char tokens
        advance(lexer);
        advance(lexer);
        advance(lexer);
    } else if (isTwoCharPunctuation) {
        t.kind = PUNCTUATION;
        t.length = 2;
        // Advance the lexer twice for two-char tokens
        advance(lexer);
        advance(lexer);
    } else if (isPunctuation) {
        t.kind = PUNCTUATION;
        t.length = 1;
        advance(lexer);
    }

    // Nothing was consumed, advance to avoid being stuck in an infinite loop
    if (lexer->c == start && t.kind == INVALID) advance(lexer);
    return t;
}

// Find a keyword string in the KEYWORD_ENUM_CONVERSION_TABLE
static inline const char* findKeywordString(KeywordKind kw) {
    return KEYWORD_ENUM_CONVERSION_TABLE[kw];
}

// Used for printing a Token's value
// Finishes in O(1) time
void printToken(Token* tok) {
    char* kindBuf[25];

    switch (tok->kind) {
        case IDENTIFIER: {
            *kindBuf = "Identifier"; 
            break;
        }
        case KEYWORD: {
            *kindBuf = "Keyword";
            break;
        }
        case NUM_LITERAL: {
            *kindBuf = "Number";
            break;
        }
        case CHAR_LITERAL: {
            *kindBuf = "Character";
            break;
        }
        case STR_LITERAL: {
            *kindBuf = "String";
            break;
        }
        case PUNCTUATION: {
            *kindBuf = "Punctuation";
            break;
        }
        case NON_SOURCE: {
            *kindBuf = "Non-Source";
            break;
        }
        case INVALID: {
            *kindBuf = "Invalid";
            break;
        }
        case NEWLINE: {
            *kindBuf = "Newline";
            break;
        }
        default: {
            *kindBuf = "";
            break;
        }
    }

    if (tok->kind == KEYWORD)
        printf("%s: %.*s (%s) - %zu:%zu (%zu)\n", *kindBuf, (int)tok->length, tok->text, findKeywordString(tok->typeValue.kw), tok->row, tok->col, tok->length);
    else if (tok->kind == NEWLINE)
        printf("%s: \\n - %zu:%zu (%zu)\n", *kindBuf, tok->row, tok->col, tok->length);
    else
        printf("%s: %.*s - %zu:%zu (%zu)\n", *kindBuf, (int)tok->length, tok->text, tok->row, tok->col, tok->length);

    return;
}