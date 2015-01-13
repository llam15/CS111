#include "alloc.h"
#include "command-internals.h"
#include "command.h"
#include "parser.h"
#include <stdint.h>

static Token_t* g_tok_list;
static uint64_t g_tok_index;
static uint64_t g_tok_list_len;

typedef struct
{
command_t root;
command_t cur_node;
} tree_context;

static uint64_t line_num;

void parse(Token_t* tok_list, uint64_t tok_list_len)
{
  root = NULL;
  cur_node = root;
  g_tok_index = 0;
  g_tok_list = tok_list;
  g_tok_list_len = tok_list_len;
  shell();
}

bool getTok(void)
{
  if (g_tok_index++ == g_tok_list_len)
    return true;
  return false;
}

//Note: Make sure that parent and child are both malloc'd
void insert_child(command_t parent, command_t child, tree_context * context)
{
  uint8_t num_children = 0;
  switch(parent->type) {
  case IF_COMMAND:
    num_children = 3;
    break;
  case PIPE_COMMAND:
    num_children = 2;
    break;
  case SEQUENCE_COMMAND:
    num_children = 2;
    break;
  case SUBSHELL_COMMAND:
    num_children = 1;
    break;
  case UNTIL_COMMAND:
    num_children = 2;
    break;
  case WHILE_COMMAND:
    num_children = 2;
    break;
  }

  if (num_children == 0)
    fprintf(stderr, "%llu: Syntax Error: Token nesting not supported for command num %d\n", line_num, parent->type);

  uint8_t child_index;
  for(child_index = 0; child_index < num_children; child_index++)
  {
    if(parent->command[child_index] == NULL)
    {
      parent->command[child_index] = child;
      return;
    }
  }

  fprintf(stderr, "%llu: Syntax Error: Too many child tokens following command num %d\n", line_num, parent->type);
}

// Note: returns the newly created node
command_t insert_node(command_type type, tree_context * context)
{

    command_t node = (command_t) checked_malloc(sizeof(command));
    node->type = type;
    if (root == NULL)
      root = node;

    if(cur_node != NULL)
    {
      insert_child(cur_node, node);
    }

    cur_node = node;
    return node;
}

void shell_inner(tree_context * context)
{
  line_num = g_tok_list[g_tok_index].line_num;
  switch (g_tok_list[g_tok_index].type) {
  case TOK_IF:
    insert_node(IF_COMMAND, context);
    break;
  case TOK_PIPE:
    insert_node(PIPE_COMMAND, context);
    break;
  case TOK_SC:
    // Create a new tree context to hold the branch
    tree_context inner_context;

    // Add the new node to the supertree
    inner_context.cur_node = inner_context.root = insert_node(SEQUENCE_COMMAND, context);

    // Call shell_inner() recursively, passing the subtree context
    shell_inner(&inner_context);
    break;
  case TOK_WORD:
    insert_node(SIMPLE_COMMAND, context);
    break;
  case TOK_LPAREN:
    insert_node(SUBSHELL_COMMAND, context);
    break;
  case TOK_UNTIL:
    insert_node(UNTIL_COMMAND, context);
    break;
  case TOK_WHILE:
    insert_node(WHILE_COMMAND, context);
    break;
  }

}

void shell()
{
    tree_context initial_context;
    initial_context.cur_node = initial_context.root = NULL;

    shell_inner(&initial_context);
}
