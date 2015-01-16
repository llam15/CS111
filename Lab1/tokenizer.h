#ifndef _TOKENIZER_h_

#define _TOKENIZER_h_

#include <stdint.h>

typedef enum {TOK_IF, TOK_THEN, TOK_FI, TOK_ELSE, TOK_WHILE, TOK_UNTIL,
	      TOK_DO, TOK_DONE, TOK_WORD, TOK_SC, TOK_PIPE, TOK_LPAREN,
	      TOK_RPAREN, TOK_LAB, TOK_RAB, TOK_NL, TOK_COL} Token_type;

typedef struct
{
  Token_type type;
  uint64_t offset;
  uint64_t line_num;
} Token_t;

typedef struct
{
    char* token_buffer;
    Token_t* tokens;
    uint64_t num_tokens;
} TokenList_t;

void lexer_init(void);

void lexer_putchar(char c);

void lexer_get_tokens(TokenList_t* tokens);
#endif //_TOKENIZER_h_



