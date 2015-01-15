#include "alloc.h"
#include "command.h"
#include "command-internals.h"
#include "tokenizer.h"
#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <inttypes.h>

static TokenList_t* g_tokens_list;

command_t check_fail_parse(Token_t* token_list, int start, int end) {
  if (token_list[end-1].type == TOK_SC)
    return parse(g_tokens_list, start, end-1);
  
  return parse(g_tokens_list, start, end);
}

command_t parse(TokenList_t* token_list, int start, int end) {
  g_tokens_list = token_list;
  command_t ret;

  // Check invalid characters at end
  switch (token_list->tokens[end-1].type){
  case TOK_SC:
  case TOK_PIPE:
  case TOK_LAB:
  case TOK_RAB:
    error(1, 0, "%"PRIu64": Syntax error: Unexpected token\n",  token_list->tokens[end-1].line_num);
  default:
    break;
  }

  int i = end-1;
  int in_command = 0;
  // Parse SEQUENCE COMMANDS
  do {
    if (in_command == 0 && token_list->tokens[i].type == TOK_SC) {
      ret = (command_t) checked_malloc(sizeof(struct command));
      ret->type = SEQUENCE_COMMAND;
      ret->input = NULL;
      ret->output = NULL;
      ret->u.command[0] = parse(token_list, start, i);
      ret->u.command[1] = parse(token_list, i+1, end);
      return ret;
    }
    else {
      switch(token_list->tokens[i].type) {
      case TOK_FI:
      case TOK_RPAREN:
      case TOK_DONE:
	in_command++;
	break;
      case TOK_IF:
      case TOK_LPAREN:
      case TOK_WHILE:
      case TOK_UNTIL:
	in_command--;
	break;
      default:
	break;
      }
    }
  } while(i-- > start);

  i = end-1;
  in_command = 0;
  // Parse PIPE COMMANDS
  do {
    if (in_command == 0 && token_list->tokens[i].type == TOK_PIPE) {
    ret = (command_t) checked_malloc(sizeof(struct command));
      ret->type = PIPE_COMMAND;
      ret->input = NULL;
      ret->output = NULL;
      ret->u.command[0] = parse(token_list, start, i);
      ret->u.command[1] = parse(token_list, i+1, end);
      return ret;
    }
    else {
      switch(token_list->tokens[i].type) {
      case TOK_FI:
      case TOK_RPAREN:
      case TOK_DONE:
	in_command++;
	break;
      case TOK_IF:
      case TOK_LPAREN:
      case TOK_WHILE:
      case TOK_UNTIL:
	in_command--;
	break;
      default:
	break;
      }
    }
  } while(i-- > start);

  // All commands are now one line. No pipes or semicolons. Parse these commands
   switch(token_list->tokens[start].type) {
   case TOK_IF:
     ret = magical_if_parser(token_list->tokens, start, end);
     break;
   case TOK_WHILE:
   case TOK_UNTIL:
     ret = magical_while_until_parser(token_list->tokens, start, end);
     break;
   case TOK_LPAREN:
     ret = magical_subshell_parser(token_list->tokens, start, end);
     break;
   case TOK_WORD:
     ret = magical_word_parser(token_list->tokens, start, end);
     break;
   default:
      error(1, 0, "%"PRIu64": Syntax error: Invalid token\n",  token_list->tokens[start].line_num);
     break;
   }

  


   return ret;
}

command_t magical_if_parser(Token_t* token_list, int start, int end) {
  command_t ret = (command_t) checked_malloc(sizeof(struct command));
  ret->type = IF_COMMAND;
  ret->input = NULL;
  ret->output = NULL;
  int index = start + 1;
  int then_index = -1;
  int else_index = -1;
  int fi_index = -1;
  int in_command = 0;
    do {
      if (in_command == 0 && index != start+1) {
	switch (token_list[index].type) {
	case TOK_THEN:
	  then_index = index;
	  break;
	case TOK_ELSE:
	  else_index = index;
	  break;
	case TOK_FI:
	  fi_index = index;
	  goto DONE_WITH_SCAN;
	  break;
	default:
	  break;
	}
      }
      //      else {
	switch(token_list[index].type) {
	case TOK_FI:
	case TOK_RPAREN:
	case TOK_DONE:
	  in_command--;
	  break;
	case TOK_IF:
	case TOK_LPAREN:
	case TOK_WHILE:
	case TOK_UNTIL:
	  in_command++;
	  break;
	default:
	  break;
	}
	//      }
    } while (index++ < end);

 DONE_WITH_SCAN:
  if (fi_index == -1) {
    error(1,0, "%"PRIu64": Syntax error: Expected `fi'\n",token_list[start].line_num);
    }
  if (then_index != -1)
    ret->u.command[0] = check_fail_parse(token_list, start+1, then_index);
  else
    error(1,0, "%"PRIu64": Syntax error: Expected `then'\n",token_list[start].line_num);

  if (else_index != -1) {
    ret->u.command[1] = check_fail_parse(token_list, then_index+1, else_index);
    ret->u.command[2] = check_fail_parse(token_list, else_index+1, fi_index);
  }
  else {
    ret->u.command[1] = check_fail_parse(token_list, then_index+1, fi_index);
    ret->u.command[2] = NULL;
  }

  add_redirects(token_list, ret, fi_index+1, end);
  return ret;
}

command_t magical_while_until_parser(Token_t* token_list, int start, int end) {
  command_t ret = (command_t) checked_malloc(sizeof(struct command));
  if (token_list[start].type == TOK_UNTIL) {
    ret->type = UNTIL_COMMAND;
    ret->input = NULL;
    ret->output = NULL;
  }
  else if (token_list[start].type == TOK_WHILE) {
    ret->type = WHILE_COMMAND;
    ret->input = NULL;
    ret->output = NULL;

  }
  else
    error(1,0, "%"PRIu64": Syntax error: Invalid token..just in case\n",token_list[start].line_num);

  int index = start+1;
  int do_index = -1;
  int done_index = -1;
  int in_command = 0;

  do {
    if (in_command == 0 && index != start+1) {
      if (token_list[index].type == TOK_DONE) {
	done_index = index;
	break;
      }

      else if (token_list[index].type == TOK_DO) {
	do_index = index;
      }
    }  
    //    else {
      switch(token_list[index].type) {
      case TOK_FI:
      case TOK_RPAREN:
      case TOK_DONE:
	in_command--;
	break;
      case TOK_IF:
      case TOK_LPAREN:
      case TOK_WHILE:
      case TOK_UNTIL:
	in_command++;
	break;
      default:
	break;
      }
      //}
  } while(index++ < end);

  if (done_index == -1) {
    error(1,0, "%"PRIu64": Syntax error: Expected `done'\n",token_list[start].line_num);
    }
  if (do_index != -1) {
    ret->u.command[0] = check_fail_parse(token_list, start+1, do_index);
    ret->u.command[1] = check_fail_parse(token_list, do_index+1, done_index);
    ret->u.command[2] = NULL;
  }
  else
    error(1,0, "%"PRIu64": Syntax error: Expected `do'\n",token_list[start].line_num);

  add_redirects(token_list, ret, done_index+1, end);
  return ret;
}

command_t magical_subshell_parser(Token_t* token_list, int start, int end) {
  command_t ret = (command_t) checked_malloc(sizeof(struct command));
  ret->type = SUBSHELL_COMMAND;
  ret->input = NULL;
  ret->output = NULL;
  int index = start+1;
  int rparen_index = -1;
  int in_command = 0;

  do {
      if (in_command == 0 && token_list[index].type == TOK_RPAREN) {
	  rparen_index = index;
	  break;
      }
      else {
	switch(token_list[index].type) {
	case TOK_FI:
	case TOK_RPAREN:
	case TOK_DONE:
	  in_command--;
	  break;
	case TOK_IF:
	case TOK_LPAREN:
	case TOK_WHILE:
	case TOK_UNTIL:
	  in_command++;
	  break;
	default:
	  break;
	}
      }
  } while (index++ < end);
  
  if (rparen_index == -1)
    error(1,0, "%"PRIu64": Syntax error: Expected `)'\n",token_list[start].line_num);
  else {
    ret->u.command[0] = check_fail_parse(token_list, start+1, rparen_index);
    ret->u.command[1] = NULL;
    ret->u.command[2] = NULL;
  }

  add_redirects(token_list, ret, rparen_index+1, end);
  return ret;
}

command_t magical_word_parser(Token_t* token_list, int start, int end) {
  command_t ret = (command_t) checked_malloc(sizeof(struct command));
  ret->type = SIMPLE_COMMAND;
  ret->input = NULL;
  ret->output = NULL;

  int index = start;

  do {
    if (token_list[index].type != TOK_WORD)
      break;
    } while (index++ < end);

  int i;
  int e = index-start;
  ret->u.word = (char**) checked_malloc(sizeof(char*)*(e+1));
  
  for(i = 0; i < e+1; i++) {
    ret->u.word[i] = g_tokens_list->token_buffer+token_list[start+i].offset;
  }
  ret->u.word[e] = NULL;

  add_redirects(token_list, ret, index, end);
  return ret;
}

void add_redirects(Token_t* token_list, command_t comm, int start, int end) {
  if (start >= end)
    return;

  if (token_list[start].type == TOK_LAB && end > start+1) {
    comm->input = g_tokens_list->token_buffer+token_list[start+1].offset;
  }
  else if (token_list[start].type == TOK_RAB && end > start+1) {
    comm->output = g_tokens_list->token_buffer+token_list[start+1].offset;
  }
    else
      error(1, 0, "%"PRIu64": Syntax error: Unexpected token while redirecting\n",token_list[start].line_num);

  add_redirects(token_list, comm, start+2, end);
}
