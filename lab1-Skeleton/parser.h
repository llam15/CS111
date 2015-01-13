#ifndef _PARSER_h_
#define _PARSER_h_

#include "tokenizer.h"
#include "command.h"
#include <stdint.h>
#include <stdbool.h>

void parse(Token_t* tok_list, uint64_t tok_list_len, command_stream_t parsed_commands);

bool getTok(void);

command_t shell(void);

#endif //PARSER_h_
