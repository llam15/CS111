#ifndef _PARSER_h_
#define _PARSER_h_

#include "tokenizer.h"
#include <stdint.h>
#include <stdbool.h>

void parse(Token_t* tok_list, uint64_t tok_list_len, command_stream_t parsed_commands);

bool getTok(void);

void shell(void);

void yolk(void);

bool expect(Token_type type);

bool accept(Token_type type);




#endif //PARSER_h_
