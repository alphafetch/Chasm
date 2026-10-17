#ifndef CHASM_LEXER_H
#define CHASM_LEXER_H

#include <stdlib.h>

typedef enum {
    IDENTIFIER,      // Letters, digits, and _, not starting with a digit, case-sensitive
    KEYWORD,         // Declarations, control flow, errors, memory, values, other keywords, types, and directives:
                     //   Declarations: var, const, unsigned, define, lambda, struct, enum, error, interface, public, private, self
                     //   Control Flow: if, other, for, foreach, in, while, do, stop, continue, break, travel, switch, is, otherwise, return, leaveprog, block
                     //         Errors: attempt, reject, err, of
                     //         Memory: new, del, defer, sizeof
                     //         Values: true, false, NULL, undefined
                     //          Other: asm
                     //          Types: void, bool, char, unichar, byte, dbyte, string, int, int8, int16, int32, int64, short, long, float,
                     //               | float32, float64, double, double32, double64, szt, pszt
                     //     Directives: get, as, use, else, end, bare (define and if above)
    NUM_LITERAL,     // Decimal, hex, binary, octal, and float
                     // f, l, s, z, u - combinable, f not on hex - because it is a hex digit
    CHAR_LITERAL,    // 'a' and '\u{1F600}' as well as escapes:
                     //   Escape Codes: \n \t \r \0 \\ \' \" \xHH \u{H...}
    STR_LITERAL,     // "..." which may span lines, raw r"..." where backslashes are literal
    PUNCTUATION,     // Brackets, arithmetic, incrementors, bitwise, comparisons, logical, ternary, assignment, prefix, hash prefixes (with ^^(hashForDoubleCaretDef))
                     //       Brackets: (, ), [, ], {, }, ,, ;, ., :
                     //     Arithmetic: +, -, *, /, %
                     //   Incrementors: ++, --
                     //        Bitwise: &, |, ^, ~, <<, >>
                     //     Comparison: <, <=, >, >=, ==, !=
                     //        Logical: !, &&, ||
                     //        Ternary: =>
                     //     Assignment: =, +=, -=, *=, /=, %=, <<=, >>=, &=, ^=, |=
                     //         Prefix: ^^
                     //    Hash Prefix: # (only with ^^(hashForDoubleCaretDef))
    NON_SOURCE,      // EOF token for the parser to stop on
    INVALID,         // Any character not above: @, $, `, a stray \, # without the attribute
    NEWLINE,         // Specifically for newline characters
} TokenKind;

typedef enum {
    // -- Declarations
    KW_VAR,          // var
    KW_CONST,        // const
    KW_UNSIGNED,     // unsigned
    KW_DEFINE,       // define
    KW_LAMBDA,       // lambda
    KW_STRUCT,       // struct
    KW_ENUM,         // enum
    KW_ERROR,        // error
    KW_INTERFACE,    // interface
    KW_PUBLIC,       // public
    KW_PRIVATE,      // private
    KW_SELF,         // self
    // -- Control Flow
    KW_IF,           // if
    KW_OTHER,        // other
    KW_FOR,          // for
    KW_FOREACH,      // foreach
    KW_IN,           // in
    KW_WHILE,        // while
    KW_DO,           // do
    KW_STOP,         // stop
    KW_CONTINUE,     // continue
    KW_BREAK,        // break
    KW_TRAVEL,       // travel 
    KW_SWITCH,       // switch
    KW_IS,           // is
    KW_OTHERWISE,    // otherwise
    KW_RETURN,       // return
    KW_LEAVEPROG,    // leaveprog
    KW_BLOCK,        // block
    // -- Error Handling
    KW_ATTEMPT,      // attempt
    KW_REJECT,       // reject
    KW_ERR,          // err
    KW_OF,           // of
    // -- Memory Management
    KW_NEW,          // new
    KW_DEL,          // del
    KW_DEFER,        // defer
    KW_SIZEOF,       // sizeof
    // -- Values
    KW_TRUE,         // true
    KW_FALSE,        // false
    KW_NULL,         // NULL
    KW_UNDEFINED,    // undefined
    // -- Other
    KW_ASM,          // asm
    // -- Types
    KW_VOID,         // void
    KW_BOOL,         // bool
    KW_CHAR,         // char
    KW_UNICHAR,      // unichar
    KW_BYTE,         // byte
    KW_DBYTE,        // dbyte
    KW_STRING,       // string
    KW_INT,          // int
    KW_INT8,         // int8
    KW_INT16,        // int16
    KW_INT32,        // int32
    KW_INT64,        // int64
    KW_SHORT,        // short
    KW_LONG,         // long
    KW_FLOAT,        // float
    KW_FLOAT32,      // float32
    KW_FLOAT64,      // float64
    KW_DOUBLE,       // double
    KW_DOUBLE32,     // double32
    KW_DOUBLE64,     // double64
    KW_SZT,          // szt
    KW_PSZT,         // pszt
    // -- Directives
    KW_GET,          // get
    KW_AS,           // as
    KW_USE,          // use
    KW_ELSE,         // else
    KW_END,          // end
    KW_BARE,         // bare
} KeywordKind;

typedef enum {
    // -- Brackets and Separators
    PC_LPAREN,       // (
    PC_RPAREN,       // )
    PC_LSQUARE,      // [
    PC_RSQUARE,      // ]
    PC_LBRACE,       // {
    PC_RBRACE,       // }
    PC_COMMA,        // ,
    PC_SEMICOLON,    // ;
    PC_DOT,          // .
    PC_COLON,        // :
    // -- Arithmetic
    PC_PLUS,         // +
    PC_MINUS,        // -
    PC_TIMES,        // *
    PC_DIVIDE,       // /
    PC_MOD,          // %
    // -- Increment
    PC_INCREMENT,    // ++
    PC_DECREMENT,    // --
    // -- Bitwise
    PC_BITAND,       // &
    PC_BITOR,        // |
    PC_BITXOR,       // ^
    PC_BITNOT,       // ~
    PC_SHIFTLEFT,    // <<
    PC_SHIFTRIGHT,   // >>
    // -- Comparison
    PC_LT,           // <
    PC_LTEQUAL,      // <=
    PC_GT,           // >
    PC_GTEQUAL,      // >=
    PC_EQUAL,        // ==
    PC_NEQUAL,       // !=
    // -- Logical
    PC_NOT,          // !
    PC_AND,          // &&
    PC_OR,           // ||
    // -- Ternary
    PC_TERNARY,      // =>
    // -- Assignment
    PC_ASSIGN,       // =
    PC_PLUSASSIGN,   // +=
    PC_MINUSASSIGN,  // -=
    PC_TIMESASSIGN,  // *=
    PC_DIVIDEASSIGN, // /=
    PC_MODASSIGN,    // %=
    PC_LEFTASSIGN,   // <<=
    PC_RIGHTASSIGN,  // >>=
    PC_BITANDASSIGN, // &=
    PC_BITXORASSIGN, // ^=
    PC_BITORASSIGN,  // |=
    // -- Prefix
    PC_DIRECTIVE,    // ^^
    // -- Hash Prefix
    PC_HASHDIRECTIVE,// #
} PunctuationKind;

typedef struct {
    TokenKind kind;         // Token type
    // This union is useless unless the token is a keyword or punctuation, otherwise it is NULL, or reads garbage
    union {                 // What it is, if applicable (ex. PC_TERNARY or KW_IF) otherwise void* to NULL
        KeywordKind kw;
        PunctuationKind pc;
        void* n;
    } typeValue;

    const char* text;       // Pointer to the text
    size_t length;          // Use length to avoid allocating memory (and therefore freeing) for the text

    int row;                // Row and column for errors
    int col;
} Token;

#endif