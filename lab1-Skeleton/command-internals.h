// UCLA CS 111 Lab 1 command internals

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

#ifndef _COMMAND_INTERNALS_H_
#define _COMMAND_INTERNALS_H_

#include <stdint.h>

typedef enum
  {
    IF_COMMAND,		 // if A then B else C fi
    PIPE_COMMAND,        // A | B
    SEQUENCE_COMMAND,    // A ; B
    SIMPLE_COMMAND,      // a simple command
    SUBSHELL_COMMAND,    // ( A )
    UNTIL_COMMAND,	 // until A do B done
    WHILE_COMMAND,	 // while A do B done
  } command_type;

// Data associated with a command.
struct command
{
  command_type type;

  // Exit status, or -1 if not known (e.g., because it has not exited yet).
  int status;

  // I/O redirections, or null if none.
  char *input;
  char *output;

  uint64_t n_words;
  uint64_t word_index;
  int child_index;

  union
  {
    // For SIMPLE_COMMAND.
    char **word;

    // For all other kinds of commands.  Trailing entries are unused.
    // Only IF_COMMAND uses all three entries.
    struct command *command[3];
  } u;
};

#endif // _COMMAND_INTERNALS_H_
