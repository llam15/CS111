#ifndef _PARSER_h_
#define _PARSER_h_

#include "tokenizer.h"
#include "command.h"
#include <stdint.h>
#include <stdbool.h>

// Pass in TokenList_t instead of all of its members duh
void parse(Token_t* tok_list, const char* tok_buffer, uint64_t tok_list_len, command_t* ret_tree);

bool getTok(void);

command_t shell(void);

#endif //PARSER_h_
