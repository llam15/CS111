#include "tokenizer.h"
#include "alloc.h"
#include <error.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>

typedef struct
{
    Token_type type;
    const char * str;
} TokenAssoc_t;

// NOTE: The association list does not include "TOK_WORD" as that is the default case
// NOTE: Colons are interpreted as TOK_WORDs
#define NUM_TOKEN_ASSOC 15

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
    {TOK_NL, "\n"}
};

// Buffer declaration and initial size
#define INIT_BUF_SIZE 64

static char* lexerbuf;
static TokenList_t lexertokens;
static uint64_t buf_index = 0;
static uint64_t last_token_index = 0;
static uint64_t line_num = 1;
static int is_command = 0;
static bool simple_command = false;

bool is_reserved(Token_t tok)
{
  switch(tok.type) {
  case TOK_WHILE:
  case TOK_UNTIL:
  case TOK_IF:
  case TOK_LPAREN:
  case TOK_THEN:
  case TOK_DO:
  case TOK_NL:
    return true;
  default: 
    return false;
  }
}

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
    if (simple_command)
      return;

    // Assign type to finished token. Also update counter on nested commands
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

    // If TOK_WORD, following tokens are also TOK_WORD until delimiter
    simple_command = true;
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
    static char last_special = '\0';

    // return to caller whilst consuming comments until a newline
    if(consume_switch)
    {
        if(c == '\n')
            consume_switch = false;
        else
            return;
    }

    switch(c) {
    // Replace whitespace with nullbytes 
    case ' ':
    case '\t':
      lexer_putchar_i('\0');
      break;
    // Error if previous character was also redirect command. Following token must be interpreted as TOK_WORD 
    case '>':
    case '<':
      if (strchr("<>", last_c))
	error(1,0,"%"PRIu64": Syntax error: Unexpected `%c'\n", line_num,c);
      lexer_putchar_i('\0');
      simple_command = false;
      lexer_putchar_i(c);
      lexer_putchar_i('\0');
      simple_command = true;
      break;
    // Error if previous character was redirect comand. Otherwise, consume unnecessary newlines. Replace with semicolon when applicable
    case '\n':
      if (strchr("<>", last_c) && last_c != '\0')
	error(1,0,"%"PRIu64": Syntax error: Unexpected newline\n", line_num);
      line_num++;
      lexer_putchar_i('\0');
      simple_command = false;
      if (strchr("\n(|\0", last_c) || is_reserved(lexertokens.tokens[lexertokens.num_tokens-1]) || (last_special == ';' && is_command != 0))
	break;

      if (is_command > 0 && !is_reserved(lexertokens.tokens[lexertokens.num_tokens-1])) {
	lexer_putchar_i(';');
	lexer_putchar_i('\0');
	break;
      }
    // Error if previous character was redirect command, Else is delimiter and ends TOK_WORDs
    case '|':
    case ';':
    case '(':
    case ')':
      if (strchr("<>", last_c))
	error(1,0,"%"PRIu64": Syntax error: Unexpected `%c'\n", line_num,c);
      lexer_putchar_i('\0');
      simple_command = false;
      lexer_putchar_i(c);
      lexer_putchar_i('\0');
      break;
    // Comment character. Consume following tokens until (but not including) newline
    case '#':
      simple_command = false;
      if(strchr(" \t\n;|()\0", last_c))
	consume_switch = true;
      else
	error(1,0,"%"PRIu64": Syntax error: Invalid character `%c'\n", line_num, c);
      break;
    // Check if c is valid word character, else return error
    default:
      if (isalnum(c) || strchr("!%+,-./:@^_", c)) {
	lexer_putchar_i(c);
      }
      else
        {
	  error(1,0, "%"PRIu64": Syntax error: `%c\' is not valid.\n",  line_num, c);
        }
      break; 
    }
    if (!strchr(" \t\n", c))
      last_special = c;

    last_c = c;
}

void lexer_get_tokens(TokenList_t* tokens)
{
  /*  
  int i;
  for (i = 0; i < lexertokens.num_tokens; i++)
    printf("%s\n", lexerbuf + lexertokens.tokens[i].offset);
  */

  // Copy lexer information to tokens
  tokens->token_buffer = lexerbuf;
  tokens->tokens = lexertokens.tokens;
  tokens->num_tokens = lexertokens.num_tokens;
}
