#ifndef _PARSER_H_
#define _PARSER_H_

#include "command.h"

command_t parse(TokenList_t* token_list, int start, int end);
command_t magical_if_parser(Token_t* token_list, int start, int end);
command_t magical_while_until_parser(Token_t* token_list, int start, int end);
command_t magical_subshell_parser(Token_t* token_list, int start, int end);
command_t magical_word_parser(Token_t* token_list, int start, int end);
void add_redirects(Token_t* token_list, command_t comm, int start, int end);

#endif //_PARSER_H_
