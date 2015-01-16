// UCLA CS 111 Lab 1 command reading

// Copyright 2012-2014 Paul Eggert.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "command.h"
#include "command-internals.h"
#include "error.h"
#include "alloc.h"
#include "tokenizer.h"
#include "parser.h"
#include <stdio.h>

struct command_stream {
  command_t command_tree_list;
  int read_index;
  int list_size;
};


command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  // Initialize lexer and tokenize input
  lexer_init();
  char c;

  // Input characters to lexer until EOF
  while ((c = (char)get_next_byte(get_next_byte_argument)) != EOF) {
    lexer_putchar(c);
  }

  // Terminate lexer buffer with null byte (to ensure last token is not lost)
  lexer_putchar('\0');

  // Get tokens
  TokenList_t token_list;
  lexer_get_tokens(&token_list);

  command_stream_t stream = (command_stream_t) checked_malloc(sizeof(struct command_stream));
  stream->list_size = 64;
  stream->command_tree_list = (command_t) checked_malloc(sizeof(struct command)*stream->list_size);

  int count = 0;
  uint64_t index = 0;
  while(1) {
    uint64_t start = index;
    // Scan until newline token to find one complete command
    while (index < token_list.num_tokens) {
      if (token_list.tokens[index].type == TOK_NL){
	break;
      }
      index++;
    }
    // Remove trailing valid semicolons if necessary, then parse
    if (token_list.tokens[index-1].type == TOK_SC)
      stream->command_tree_list[count] = *parse(&token_list, start, index-1);
    else
      stream->command_tree_list[count] = *parse(&token_list, start, index);
    index++;
    count++;
    
    // Done parsing all commands
    if (index >= token_list.num_tokens-1){
      stream->list_size = count;
      stream->read_index = 0;
      break;
    }

    // Reallocate list of command_trees if needed
    if (count == stream->list_size){
      stream->list_size *=2;
      stream->command_tree_list = (command_t) checked_realloc(stream->command_tree_list, sizeof(struct command)*stream->list_size);
    }
  }

  return stream;
}

command_t
read_command_stream (command_stream_t s)
{
  // Return next command in array
  if (s->read_index < s->list_size) {
    return s->command_tree_list + s->read_index++;
  }

  return NULL;
}
