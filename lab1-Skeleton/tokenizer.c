#include "tokenizer.h"
#include "alloc.h"
#include <stdint.h>

#define INIT_BUF_SIZE 64

static char* lexerbuf;
//static char** lexertokens;
static Tokens_t lexertokens;

void lexer_init(void) {
  lexerbuf = (char*) checked_malloc(sizeof(char)*INIT_BUF_SIZE);
  lexertokens.token_list = (char**) checked_malloc(sizeof(char*)*INIT_BUF_SIZE);
  lexertokens.num_tokens = 0;
}

void lexer_putchar_i(char c) {
  static uint64_t buf_size = INIT_BUF_SIZE;
  static uint64_t last_token_index = 0;
  static uint64_t token_list_size = INIT_BUF_SIZE;
  if (c == '\0') {
    // If there was a null byte before this one, ignore it
    if(buf_index >= 1 && lexerbuf[buf_index-1] == '\0') {
      return;
    }
    // Otherwise, update lexertokens
    else {
      //Reallocate token_list if needed
      if (lexertokens.num_tokens == token_list_size) {
	token_list_size *=2;
	lexertokens.token_list = (char**) checked_realloc(lexertokens.token_list, sizeof(char*)*INIT_BUF_SIZE);
      }
      lexertokens.token_list[lexertokens.num_tokens++] = lexerbuf + last_token_index;
      last_token_index = buf_index + 1;
    }
  }

  lexerbuf[buf_index++] = c;

  //Reallocate lexerbuf if needed
  if (buf_index == buf_size) {
    buf_size*=2;
    lexerbuf = (char*) checked_realloc(lexerbuf, sizeof(char)*buf_size));
  }
}

void lexer_putchar(char c) {
  static uint64_t buf_index = 0;

  switch(c) {
  case ' ':
  case '\t':
    lexer_putchar_i('\0');
    break;

  case '>':
  case '<':
  case '|':
  case ';':
  case '\n':
  case '(':
  case ')':
    lexer_putchar_i('\0');
    lexer_putchar_i(c);
    lexer_putchar_i('\0');
    break;
  default:
    lexer_putchar_i(c);
    break;
  }

}

void lexer_get_tokens(Tokens_t* tokens) {
  tokens->token_list = lexertokens.token_list;
  tokens->num_tokens = lexertokens.num_tokens;
}
