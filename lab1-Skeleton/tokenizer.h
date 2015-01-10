#ifndef _TOKENIZER_h_

#define _TOKENIZER_h_

#include <stdint.h>

typedef struct {
  char** token_list;
  uint64_t num_tokens;
} Tokens_t;

void lexer_init(void);

void lexer_putchar(char c);

void lexer_get_tokens(Tokens_t* tokens);
#endif //_TOKENIZER_h_



