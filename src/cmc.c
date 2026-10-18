/**
 * @file cmc.c
 * @brief Main compiler for Chasm
 */

#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

#define ULL unsigned long long

int main(int argc, char* argv[]) {
    if (argc > 1) {
        FILE* fp = fopen(argv[1], "rb");
        if (!fp) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        if (fseek(fp, 0, SEEK_END) != 0) {
            fclose(fp);
            exit(EXIT_FAILURE);
        }

        ULL fsize = ftell(fp);
        if ((long long)fsize == -1LL) {
            fclose(fp);
            exit(EXIT_FAILURE);
        }
        rewind(fp);

        // No safety on file size wrapping because ULLONG_MAX is too big for any normal filesystem
        char* buf = malloc((size_t)fsize + 1); 
        if (!buf) {
            fclose(fp);
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        // File size in bytes
        size_t bytesRead = fread(buf, 1, (size_t)fsize, fp);
        fclose(fp);

        if (bytesRead != (size_t)fsize) {
            free(buf);
            exit(EXIT_FAILURE);
        }

        buf[fsize] = '\0';

        Lexer lexer = createLexer(buf);
        char* kindBuf[25];

        // Temporary - for printing the lexed tokens
        while (*(lexer.c) != '\0') {
            Token t = nextToken(&lexer);
            
            switch (t.kind) {
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
            }

            printf("%s: %.*s - %zu:%zu (%zu)\n", *kindBuf, (int)t.length, t.text, t.row, t.col, t.length);
        }

        free(buf);
    } else {
        fprintf(stderr, "No file specified.\n");
    }

    exit(EXIT_SUCCESS);
}
