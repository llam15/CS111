#ifndef _TOKENIZER_h_

#define _TOKENIZER_h_

#include <stdint.h>

/*typedef enum
    {
        TOKEN_COMMAND,
        TOKEN_OTHER
    }Token_type_e;
*/
typedef struct
{
  //    Token_type_e type;
    uint64_t offset;
} Token_t;

typedef struct
{
    char* token_buffer;
    Token_t* tokens;
    uint64_t num_tokens;
} Tokens_t;

void lexer_init(void);

void lexer_putchar(char c);

void lexer_get_tokens(Tokens_t* tokens);
#endif //_TOKENIZER_h_



