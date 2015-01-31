// UCLA CS 111 Lab 1 command interface

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

#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "command-internals.h"
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

typedef struct command *command_t;
typedef struct command_stream *command_stream_t;

typedef struct
{
  struct timespec finish_time;
  struct timespec real_time_start;
  struct timespec real_time_end;
  struct rusage usage_times;
  char **command;
  int pid;
} profile_times;

/* Create a command stream from GETBYTE and ARG.  A reader of
   the command stream will invoke GETBYTE (ARG) to get the next byte.
   GETBYTE will return the next input byte, or a negative number
   (setting errno) on failure.  */
command_stream_t make_command_stream (int (*getbyte) (void *), void *arg);

/* Prepare for profiling to the file FILENAME.  If FILENAME is null or
   cannot be written to, set errno and return -1.  Otherwise, return a
   nonnegative integer flag useful as an argument to
   execute_command.  */
int prepare_profiling (char const *filename);

/* Read a command from STREAM; return it, or NULL on EOF.  If there is
   an error, report the error and exit instead of returning.  */
command_t read_command_stream (command_stream_t stream);

/* Print a command to stdout, for debugging.  */
void print_command (command_t);

/* Execute a command.  Use profiling according to the flag; do not profile
   if the flag is negative.  */
void execute_command (command_t, int);

/* Return the exit status of a command, which must have previously
   been executed.  Wait for the command, if it is not already finished.  */
int command_status (command_t);

void recursive_execute(command_t, int, int);

void execute_sequence(command_t, int, int);

void execute_pipe(command_t, int, int);

void execute_subshell(command_t, int, int);

void execute_if(command_t, int, int);

void execute_while(command_t, int, int);

void execute_until(command_t, int, int);

void execute_simple(command_t, int, int);

void write_log(const profile_times*);


#endif // _COMMAND_H_
