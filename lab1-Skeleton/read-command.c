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

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

#include "tokenizer.h"
#include "parser.h"
#include <stdbool.h>
#include <stdio.h>

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

struct command_stream {
  command_t command_tree;
};
static bool printed;

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */

  // Initialize lexer and tokenize input
  lexer_init();
  char c;
  while ((c = (char)get_next_byte(get_next_byte_argument)) != EOF) {
    lexer_putchar(c);
  }

  TokenList_t tokens;
  lexer_get_tokens(&tokens);

  // Parse tokens into commands
  command_stream_t parsed_commands = (command_stream_t) checked_malloc(sizeof(struct command_stream));

  parse(tokens.tokens, tokens.token_buffer, tokens.num_tokens, parsed_commands->command_tree);
  printed = false;
  //  error (1, 0, "command reading not yet implemented");
  return parsed_commands;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */

  if (!printed) {
    return s->command_tree;
    printed = true;
  }

  //  error (1, 0, "command reading not yet implemented");
  return NULL;
}
