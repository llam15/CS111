// UCLA CS 111 Lab 1 command execution

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

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

int
prepare_profiling (char const *name)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  error (0, 0, "warning: profiling not yet implemented");
  return -1;
}

int
command_status (command_t c)
{
  return c->status;
}

void
execute_command (command_t c, int profiling)
{
  /* FIXME: Replace this with your implementation, like 'prepare_profiling'.  */
  //  error (1, 0, "command execution not yet implemented");

  recursive_execute(c, -1, -1);
}

void
recursive_execute(command_t c, int input, int output) 
{
  switch(c->type) {
  case SEQUENCE_COMMAND:
    execute_sequence(c, input, output);
    break;
  case PIPE_COMMAND:
    execute_pipe(c, input, output);
    break;
  case SUBSHELL_COMMAND:
    execute_subshell(c, input, output);
    break;
  case IF_COMMAND:
    execute_if(c, input, output);
    break;
  case WHILE_COMMAND:
    execute_while(c, input, output);
    break;
  case UNTIL_COMMAND:
    execute_until(c, input, output);
    break;
  case SIMPLE_COMMAND:
    execute_simple(c, input, output);
    break;
  default:
    error(1,0,"Internal Error");
  }

}

int exec_wait(char* const * cname) {
  int status;

  pid_t pid = fork();
  if(pid < 0) {
      error(1, 0, "Failed to fork");
  }

  else if(pid > 0) {
    waitpid(pid, &status, 0);
    return WEXITSTATUS(&status);
  }

  else {
    execvp(cname[0], cname);
    fprintf(stderr, "%s: Command not found\n", cname[0]);
    _exit(127);
  }
  return -1;
}

void execute_sequence(command_t c, int input, int output)
{
  // Execute first command, then second command
  recursive_execute(c->u.command[0], input, output);
  recursive_execute(c->u.command[1], input, output);

  // Set exit status to exit status of second command
  c->status = command_status(c->u.command[1]);
}

void execute_pipe(command_t c, int input, int output)
{
  // SOMETHING HERE

  c->status = command_status(c->u.command[1]);
}

void execute_subshell(command_t c, int input, int output)
{


}

void execute_if(command_t c, int input, int output)
{
  // Execute conditional
  recursive_execute(c->u.command[0], input, output);

  // If conditional is true, execute then statement
  if (command_status(c->u.command[0]) == 0) {
    recursive_execute(c->u.command[1], input, output);
    c->status = command_status(c->u.command[1]);
  }

  // If conditional is false, execute else statement (if exists)
  else if (c->u.command[2] != NULL) {
    recursive_execute(c->u.command[2], input, output);
    c->status = command_status(c->u.command[2]);
  }

  // Conditional is false, no else statement. Exit status is conditional's exit status
  else {
    c->status = command_status(c->u.command[0]);
  }
}

void execute_while(command_t c, int input, int output)
{
  // Execute conditional. While conditional is true, execute body
  do { 
    recursive_execute(c->u.command[0], input, output);
    if (command_status(c->u.command[0]) == 0)
	recursive_execute(c->u.command[1], input, output);
  } while (command_status(c->u.command[0]) == 0);

  // Set exit status to appropriate body/conditional exit status
  if (command_status(c->u.command[1]) != -1)
    c->status = command_status(c->u.command[1]);
  else 
    c->status = command_status(c->u.command[0]);
}

void execute_until(command_t c, int input, int output)
{
  // Execute conditional. While conditional is false, execute body
  do { 
    recursive_execute(c->u.command[0], input, output);
    if (command_status(c->u.command[0]) != 0)
	recursive_execute(c->u.command[1], input, output);
  } while (command_status(c->u.command[0]) != 0);

  // Set exit status to appropriate body/conditional exit status
  if (command_status(c->u.command[1]) != -1)
    c->status = command_status(c->u.command[1]);
  else 
    c->status = command_status(c->u.command[0]);
}

void execute_simple(command_t c, int input, int output)
{
  // Do something with input output
  if (c->input != NULL) {
  }


  // Execute simple command, set exit status
  int exit = exec_wait(c->u.word);
  c->status = exit;
}

