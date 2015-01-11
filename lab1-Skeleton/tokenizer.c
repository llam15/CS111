#include "tokenizer.h"
#include "alloc.h"
#include <stdint.h>
#include <stdbool.h>

// String constants
#define NUM_RESERVED_STRINGS 8

const char* const reserved_strings[] =
{
    "if",
    "else",
    "then",
    "fi",
    "while",
    "until",
    "do",
    "done"
};

// Buffer declaration and initial size
#define INIT_BUF_SIZE 64

static char* lexerbuf;
static Tokens_t lexertokens;
static uint64_t buf_index = 0;
static uint64_t last_token_index = 0;

bool is_reserved(const char* str, uint64_t strl)
{
    int i = 0;
    for(; i < NUM_RESERVED_STRINGS; i++)
        if(strlen(reserved_strings[i]) == strl && memcmp(str, reserved_strings[i], strl) == 0)
            return true;
    return false;
}

// Initialize the lexer
void lexer_init(void)
{
    // Allocate buffers
    lexerbuf = (char*) checked_malloc(sizeof(char)*INIT_BUF_SIZE);
    lexertokens.tokens = (Token_t*) checked_malloc(sizeof(Token_t)*INIT_BUF_SIZE);
    lexertokens.num_tokens = 0;
}

// Internal putchar function which resizes buffers if necessary
void lexer_putchar_i(char c)
{
    static uint64_t buf_size = INIT_BUF_SIZE;
    static uint64_t tokens_count = INIT_BUF_SIZE;

    if (c == '\0')
    {
        // If there was a null byte before this one, ignore it
        if(buf_index >= 1 && lexerbuf[buf_index-1] == '\0')
        {
            return;
        }
        // Otherwise, update lexertokens
        else
        {
            //Reallocate token_offsets if needed
            if (lexertokens.num_tokens == tokens_count)
            {
                tokens_count *= 2;
                lexertokens.tokens = (Token_t*) checked_realloc(lexertokens.tokens, sizeof(Token_t)*tokens_count);
            }
            lexertokens.tokens[lexertokens.num_tokens++].offset = last_token_index;
            last_token_index = buf_index + 1;
        }
    }

    lexerbuf[buf_index++] = c;

    //Reallocate lexerbuf if needed
    if (buf_index == buf_size)
    {
        buf_size*=2;
        lexerbuf = (char*) checked_realloc(lexerbuf, sizeof(char)*buf_size);
    }
}

void lexer_putchar(char c)
{

    switch(c)
    {
    case ' ':
    case '\t':
        if(lexertokens.tokens[lexertokens.num_tokens].type != TOKEN_COMMAND && is_reserved(lexerbuf + last_token_index, buf_index - last_token_index))
            lexer_putchar_i('\0');
        else
            lexertokens.tokens[lexertokens.num_tokens].type = TOKEN_COMMAND;
        break;

    case '>':
    case '<':
    case '|':
    case ';':
    case '\n':
    case '(':
    case ')':
        lexer_putchar_i('\0');
        lexer_putchar_i((c == '\n')?';':c);
        lexer_putchar_i('\0');
        break;
    default:
        lexer_putchar_i(c);
        break;
    }

}

void lexer_get_tokens(Tokens_t* tokens)
{
    tokens->token_buffer = lexerbuf;
    tokens->tokens = lexertokens.tokens;
    tokens->num_tokens = lexertokens.num_tokens;
}
