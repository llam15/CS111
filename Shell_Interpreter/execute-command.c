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

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdbool.h>
#include <math.h>

static int can_write;
static int log_file;
static int monotonic_res;
static int realtime_res;

#define BILLION (1000000000.0)
#define MILLION (1000000.0)

int
prepare_profiling (char const *name)
{
  can_write = 1;
  return open(name, O_WRONLY | O_CREAT | O_APPEND,
	      S_IRUSR | S_IWUSR | S_IXUSR);
}

void
write_log(const profile_times *times)
{
  if (!can_write)
    return;

  char buf[1024];
  int num_chars = -1;

  // Calculate times by adding seconds and nanoseconds together. Cast to double
  double completion_time = times->finish_time.tv_sec + (times->finish_time.tv_nsec/BILLION);
  double real_sec  = times->real_time_end.tv_sec  - times->real_time_start.tv_sec;
  long real_nsec   = times->real_time_end.tv_nsec - times->real_time_start.tv_nsec;
  double real_time = real_sec + (real_nsec/BILLION);
  double user_time = times->usage_times.ru_utime.tv_sec + (times->usage_times.ru_utime.tv_usec/MILLION);
  double sys_time  = times->usage_times.ru_stime.tv_sec + (times->usage_times.ru_stime.tv_usec/MILLION);

  // If has command name, print times with command
  if (times->command == NULL) {
     num_chars = snprintf(buf, 1023, "%.*f %.*f %0.6f %0.6f [%d]", realtime_res, completion_time, monotonic_res, real_time, 
			  user_time, sys_time, times->pid);
     if (num_chars < 0)
       error(1, 0, "Error while writing to log.\n");
  }

  // If no command name, print process number
  else {
    num_chars = snprintf(buf, 1023, "%.*f %.*f %0.6f %0.6f %s", realtime_res, completion_time, monotonic_res, real_time, 
			 user_time, sys_time, times->command[0]);
    if (num_chars < 0)
      error(1,0, "Error while writing to log.\n");

    // Print all arguments
    int i = 1;
    while (times->command[i] != NULL) {
      int added;
      if (1023-num_chars > 0)
	added = snprintf(buf + num_chars, 1023-num_chars, " %s", times->command[i]);

      if (added < 0)
	error(1,0, "Error while writing to log.\n");
      else
	num_chars += added;
      i++;
    }
    if (num_chars < 0)
      error(1,0, "Error while writing to log.\n");      
  }

  // Lock file to prevent interweaving
  struct flock lock;
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;
  lock.l_pid = getpid();

  fcntl(log_file, F_SETLKW, &lock);

  // Print profile times
  int num_bytes =  num_chars > 1023 ? 1023 : num_chars;
  int written = write(log_file, buf, num_bytes);
  write(log_file, "\n", 1);

  // If cannot write to file, stop trying to write
  if (written != num_bytes)
    can_write = 0;

  // Unlock file
  lock.l_type = F_UNLCK;
  fcntl(log_file, F_SETLK, &lock);
}

int
get_write_status(void)
{
  return can_write;
}

int
command_status (command_t c)
{
  return c->status;
}

void
execute_command (command_t c, int profiling)
{
  // Get system resolution
  struct timespec monotonic;
  struct timespec realtime;
  clock_getres(CLOCK_MONOTONIC, &monotonic);
  clock_getres(CLOCK_REALTIME, &realtime);

  // Calculate resolution decimal precision 
  monotonic_res = (int) (9 - log10(monotonic.tv_nsec));
  realtime_res = (int) (9 - log10(realtime.tv_nsec));

  // Set file descriptor
  log_file = profiling;

  // Execute commands
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
  int status;
  int fd[2];
  pid_t pid_left, pid_right;
  profile_times pt_left, pt_right;

  pt_left.command = NULL;
  pt_right.command = NULL;
  
  // Create pipe
  pipe(fd);

  // Beginning of real time
  clock_gettime(CLOCK_MONOTONIC, &pt_left.real_time_start);

  // Fork for left side of pipe
  pid_left = fork();

  // Check fork
  if (pid_left < 0)
      error(1, 0, "Fatal Error: Failed to fork");
  
  // Execute left child
  else if (pid_left == 0) {
    close(fd[0]);
    recursive_execute(c->u.command[0], input, fd[1]);
    exit(command_status(c->u.command[0]));
  }
  
  // Parent. Fork again for right child
  else {
    clock_gettime(CLOCK_MONOTONIC, &pt_right.real_time_start);
    pid_right = fork();

    // Check fork
    if (pid_right < 0)
      error(1, 0 , "Fatal Error: Failed to fork");

    // Execute right child
    else if (pid_right == 0) {
      close(fd[1]);
      recursive_execute(c->u.command[1], fd[0], output);
      exit(command_status(c->u.command[1]));
    }

    //Parent. Wait for left and right children.
    else {
      close(fd[0]);
      close(fd[1]);

      wait4(pid_left, &status, 0, &pt_left.usage_times);
      clock_gettime(CLOCK_MONOTONIC, &pt_left.real_time_end);
      clock_gettime(CLOCK_REALTIME, &pt_left.finish_time);
      pt_left.pid = pid_left;
      
      wait4(pid_right, &status, 0, &pt_right.usage_times);
      clock_gettime(CLOCK_MONOTONIC, &pt_right.real_time_end);
      clock_gettime(CLOCK_REALTIME, &pt_right.finish_time);
      pt_right.pid = pid_right;
      
      write_log(&pt_left);
      write_log(&pt_right);
      
      if (WIFEXITED(status))
	c->u.command[1]->status = WEXITSTATUS(status);
      else if (WIFSIGNALED(status))
	c->u.command[1]->status = WTERMSIG(status);
      else
	c->u.command[1]->status = WSTOPSIG(status);
    }
  }
  c->status = command_status(c->u.command[1]);
}

void execute_subshell(command_t c, int input, int output)
{
  // Open input/output files
  if (c->input != NULL)
    input = open(c->input, O_RDONLY);
  if (c->output != NULL)
    output = open(c->output, O_WRONLY | O_TRUNC | O_CREAT, 
		  S_IRUSR | S_IWUSR | S_IXUSR);

  // Fork
  int status;
  profile_times pt;
  pt.command = NULL;

  clock_gettime(CLOCK_MONOTONIC, &pt.real_time_start);
  
  pid_t pid = fork();

  // Check fork
  if (pid < 0)
    error(1, 0 , "Fatal Error: Failed to fork");

  // Child. Execute commands
  else if (pid  == 0 ) {
    recursive_execute(c->u.command[0], input, output);
    exit(command_status(c->u.command[0]));
  }

  // Parent. Wait for child. Set exit status to child exit status
  else {
    wait4(pid, &status, 0, &pt.usage_times);
    clock_gettime(CLOCK_MONOTONIC, &pt.real_time_end);
    clock_gettime(CLOCK_REALTIME, &pt.finish_time);
    pt.pid = pid;
    write_log(&pt);
    
    if (WIFEXITED(status))
      c->status = WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
      c->status = WTERMSIG(status);
    else
      c->status = WSTOPSIG(status);
  }

  // Close any open input/output files
  if (c->input != NULL)
    close(input);
  if (c->output != NULL)
    close(output);
}

void execute_if(command_t c, int input, int output)
{
  // Open input/output files
  if (c->input != NULL)
    input = open(c->input, O_RDONLY);
  if (c->output != NULL)
    output = open(c->output, O_WRONLY | O_TRUNC | O_CREAT, 
		  S_IRUSR | S_IWUSR | S_IXUSR);

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

  // Conditional is false, no else statement. 
  // Exit status is conditional's exit status
  else {
    c->status = command_status(c->u.command[0]);
  }

  // Close any open input/output file
  if (c->input != NULL)
    close(input);
  if (c->output != NULL)
    close(output);
}

void execute_while(command_t c, int input, int output)
{
  // Open input/output files
  if (c->input != NULL)
    input = open(c->input, O_RDONLY);
  if (c->output != NULL)
    output = open(c->output, O_WRONLY | O_TRUNC | O_CREAT, 
		  S_IRUSR | S_IWUSR | S_IXUSR);

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

  // Close any open input/output file
  if (c->input != NULL)
    close(input);
  if (c->output != NULL)
    close(output);
}

void execute_until(command_t c, int input, int output)
{
  // Open input/output files
  if (c->input != NULL)
    input = open(c->input, O_RDONLY);
  if (c->output != NULL)
    output = open(c->output, O_WRONLY | O_TRUNC | O_CREAT, 
		  S_IRUSR | S_IWUSR | S_IXUSR);

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

  // Close any open input/output file
  if (c->input != NULL)
    close(input);
  if (c->output != NULL)
    close(output);
}

void execute_simple(command_t c, int input, int output)
{
  // Open input/output files
  if (c->input != NULL)
    input = open(c->input, O_RDONLY);
  if (c->output != NULL)
    output = open(c->output, O_WRONLY | O_TRUNC | O_CREAT, 
		  S_IRUSR | S_IWUSR | S_IXUSR);

  profile_times pt;
  pt.command = c->u.word;

  clock_gettime(CLOCK_MONOTONIC, &pt.real_time_start);

  //If command is `:', ignore command
  if (!strcmp(c->u.word[0], ":")) { 
    clock_gettime(CLOCK_MONOTONIC, &pt.real_time_end);
    clock_gettime(CLOCK_REALTIME, &pt.finish_time);
    memset(&pt.usage_times, 0, sizeof(struct rusage));
    write_log(&pt);
    c->status = 0;
    return;
  }

  // If the comand is an exec command, replace shell without forking
  if (!strcmp(c->u.word[0], "exec")) {
    // Redirect input if needed
    if (input != -1)
      dup2(input, STDIN_FILENO);

    // Redirect output if needed
    if (output != -1)
      dup2(output, STDOUT_FILENO);

    // Execute command
    execvp(c->u.word[1], c->u.word+1);
    fprintf(stderr, "%s: Command not found\n", c->u.word[1]);
    _exit(127);
  }
  
  // Fork
  int status;
  pid_t pid = fork();

  // Error while forking
  if (pid < 0) {
      error(1, 0, "Fatal Error: Failed to fork");
  }

  // Child. Execute command
  else if (pid == 0) {
    // Redirect input if needed
    if (input != -1)
      dup2(input, STDIN_FILENO);

    // Redirect output if needed
    if (output != -1)
      dup2(output, STDOUT_FILENO);

    // Execute command
    execvp(c->u.word[0], c->u.word);
    fprintf(stderr, "%s: Command not found\n", c->u.word[0]);
    _exit(127);
  }

  // Parent. Wait for child. Set exit status to child exit status
  else {
    wait4(pid, &status, 0, &pt.usage_times);
    clock_gettime(CLOCK_MONOTONIC, &pt.real_time_end);
    clock_gettime(CLOCK_REALTIME, &pt.finish_time);
    write_log(&pt);

    if (WIFEXITED(status))
      c->status = WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
      c->status = WTERMSIG(status);
    else
      c->status = WSTOPSIG(status);
  }

  // Close any open input/output file
  if (c->input != NULL)
    close(input);
  if (c->output != NULL)
    close(output);
}

