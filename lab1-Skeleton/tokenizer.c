#include "tokenizer.h"
#include "alloc.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>

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

typedef struct
{
    Token_type type;
    const char * str;
} TokenAssoc_t;

// NOTE: the association list does not include "TOK_WORD" as that is the default case
#define NUM_TOKEN_ASSOC 16

const TokenAssoc_t const TokAssociations[] =
{
    {TOK_IF, "if"},
    {TOK_THEN, "then"},
    {TOK_FI, "fi"},
    {TOK_ELSE, "else"},
    {TOK_WHILE, "while"},
    {TOK_UNTIL, "until"},
    {TOK_DO, "do"},
    {TOK_DONE, "done"},
    {TOK_SC, ";"},
    {TOK_PIPE, "|"},
    {TOK_LPAREN, "("},
    {TOK_RPAREN, ")"},
    {TOK_LAB, "<"},
    {TOK_RAB, ">"},
    {TOK_NL, "\n"},
    {TOK_COL, ":"},
};

// Buffer declaration and initial size
#define INIT_BUF_SIZE 64

static char* lexerbuf;
static TokenList_t lexertokens;
static uint64_t buf_index = 0;
static uint64_t last_token_index = 0;
static uint64_t line_num = 1;
static int is_command = 0;

/*bool is_reserved(const char* str, uint64_t strl)
{
    int i = 0;
    for(; i < NUM_RESERVED_STRINGS; i++)
        if(strlen(reserved_strings[i]) == strl && memcmp(str, reserved_strings[i], strl) == 0)
            return true;
    return false;
    }*/

// Initialize the lexer
void lexer_init(void)
{
    // Allocate buffers
    lexerbuf = (char*) checked_malloc(sizeof(char)*INIT_BUF_SIZE);
    lexertokens.tokens = (Token_t*) checked_malloc(sizeof(Token_t)*INIT_BUF_SIZE);
    lexertokens.num_tokens = 0;
}

void lexer_assign_type(Token_t* tok)
{
    size_t i;
    tok->type = TOK_WORD;

    for(i = 0; i < NUM_TOKEN_ASSOC; i++)
    {
        if(strcmp(TokAssociations[i].str, lexerbuf + tok->offset) == 0) // FIX DIS SHIT NEED LENGTHS FOR REASONS
        {
            tok->type = TokAssociations[i].type;
	    switch(tok->type) {
	    case TOK_IF:
	    case TOK_WHILE:
	    case TOK_UNTIL:
	    case TOK_LPAREN:
	      is_command++;
	      break;
	    case TOK_FI:
	    case TOK_DONE:
	    case TOK_RPAREN:
	      is_command--;
	      break;
	    default:
	      break;
	    }

            return;
        }
    }
}

// Internal putchar function which resizes buffers if necessary
void lexer_putchar_i(char c)
{
    static uint64_t buf_size = INIT_BUF_SIZE;
    static uint64_t tokens_count = INIT_BUF_SIZE;

    if (c == '\0')
    {
        // If there was a null byte before this one, ignore it; additionally, ignore leading null bytes
        if(((buf_index >= 1) && (lexerbuf[buf_index-1] == '\0')) || buf_index == 0)
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

            lexerbuf[buf_index] = '\0';

            //Set params on the just-finished token
            lexertokens.tokens[lexertokens.num_tokens].offset = last_token_index;

            //Assign its type
            lexer_assign_type(lexertokens.tokens + lexertokens.num_tokens);

            lexertokens.tokens[lexertokens.num_tokens].line_num = line_num;

            lexertokens.num_tokens++;

            last_token_index = buf_index + 1;

            buf_index++;
        }
    }
    else
    {
        lexerbuf[buf_index++] = c;
    }

    //Reallocate lexerbuf if needed
    if (buf_index == buf_size)
    {
        buf_size*=2;
        lexerbuf = (char*) checked_realloc(lexerbuf, sizeof(char)*buf_size);
    }
}

void lexer_putchar(char c)
{
    static bool consume_switch = false;
    static char last_c = '\0';

    // return to caller whilst consuming (comments?) until a newline
    if(consume_switch)
    {
        if(c == '\n')
            consume_switch = false;
        else
            return;
    }

    switch(c)
    {
    case ' ':
    case '\t':
        lexer_putchar_i('\0');
        break;
    case '\n':
      if (is_command > 0)
	c = ';';
        line_num++;
    case '>':
    case '<':
    case '|':
    case ';':
    case '(':
    case ')':
        lexer_putchar_i('\0');
        lexer_putchar_i(c);
        lexer_putchar_i('\0');
        break;
    // Comment character
    case '#':
        if(strchr(" \t\n;|()", last_c))
            consume_switch = true;
        break;
    default:
        if (isalnum(c) || strchr("!%+,-./:@^_", c))
            lexer_putchar_i(c);
        else
        {
            fprintf(stderr, "%"PRIu64": Syntax error: `%c\' is not valid.\n", (unsigned long long) line_num, c);
            exit(1);
        }

        break;
    }

    last_c = c;
}

void lexer_get_tokens(TokenList_t* tokens)
{
    tokens->token_buffer = lexerbuf;
    tokens->tokens = lexertokens.tokens;
    tokens->num_tokens = lexertokens.num_tokens;
}
