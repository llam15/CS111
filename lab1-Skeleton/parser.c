#include "alloc.h"
#include "command-internals.h"
#include "command.h"
#include "parser.h"
#include <stdint.h>
#include <stdio.h>

#define DEFAULT_WORD_NUM (4)

static Token_t* g_tok_list;
static uint64_t g_tok_index;
static uint64_t g_tok_list_len;
static const char* g_tok_buffer;

typedef struct
{
    command_t root;
    command_t cur_node;
} tree_context;

static uint64_t line_num;

void parse(Token_t* tok_list, const char* tok_buffer, uint64_t tok_list_len)
{
    g_tok_buffer = tok_buffer;
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
void insert_child(command_t parent, command_t child)
{
    uint8_t num_children = 0;
    switch(parent->type)
    {
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
    default:
        break;
    }

    if (num_children == 0)
        fprintf(stderr, "%llu: Syntax Error: Token nesting not supported for command num %d\n", line_num, parent->type);

    uint8_t child_index;
    for(child_index = 0; child_index < num_children; child_index++)
    {
        if(parent->u.command[child_index] == NULL)
        {
            parent->u.command[child_index] = child;
            return;
        }
    }

    fprintf(stderr, "%llu: Syntax Error: Too many child tokens following command num %d\n", line_num, parent->type);
}

// Malloc/construct a new node, setting all child pointers to NULL in preparation for child insertion
command_t construct_node(command_type type)
{
    // checked malloc the actual command struct
    command_t ret = (command_t) checked_malloc(sizeof(struct command));

    // initialize the type
    ret->type = type;
    if(type != SIMPLE_COMMAND)
    {
        // If it is not a simple_command, initialize the contents of the buffer
        int i;
        for(i = 0; i < 3; i++)
        {
            ret->u.command[i] = NULL;
        }
    }
    else
    {
        // Malloc space for the SIMPLE_COMMAND's word
        ret->n_words = DEFAULT_WORD_NUM;
        ret->word_index = 0;
        ret->u.word = (char**)checked_malloc(sizeof(char*) * (ret->n_words + 1));

        // Initialize newly allocated pointers to NULL
        int i;
        for(i = 0; i < (ret->n_words + 1); i++)
        {
            ret->u.words[i] = NULL;
        }
    }
    return ret;
}

// Attach another word to the end of the words pointer list on a simple_command
void simple_append_word(command_t simple_command, char* word)
{
    if(simple_command->type != SIMPLE_COMMAND)
    {
        fprintf(stderr, "%llu: Internal error: attempt to append %s to non-simple command\n", line_num, word);
        return;
    }
    // If the word index overruns the buffer, attempt to reallocate
    if(simple_command->word_index == simple_command->n_words)
    {
        // Expand the buffer exponentially by two
        simple_command->n_words *= 2;
        // Checked-realloc (allocate one extra spot for a terminating null pointer)
        simple_command->u.words = (char**)checked_realloc(simple_command->u.words, sizeof(char*) * (simple_command->n_words + 1));

        // Initialize newly allocated pointers to NULL
        int i;
        for(i = simple_command->word_index + 1; i < (simple_command->n_words + 1); i++)
        {
            simple_command->u.words[i] = NULL;
        }
    }
    // Put the word at the next position
    simple_command->u.words[simple_command->word_index++] = word;
}

// Note: returns the newly created node
command_t insert_node(command_type type, tree_context * context)
{

    command_t node = construct_node(type);

    if (context->root == NULL)
        context->root = node;

    if(context->cur_node != NULL)
    {
        insert_child(context->cur_node, node);
    }

    context->cur_node = node;
    return node;
}

void shell_inner(tree_context * context)
{
    do
    {
        // Create a new tree context to hold the branch
        tree_context inner_context;

        // Update the line number
        line_num = g_tok_list[g_tok_index].line_num;

        // If the parent node is a SIMPLE_COMMAND
        if(context->root->type == SIMPLE_COMMAND)
        {
            // Only a few special tokens will break out of the SIMPLE_COMMAND argument list
            switch(g_tok_list[g_tok_index].type)
            {
            case TOK_SC:
            case TOK_PIPE:
            case TOK_LPAREN:
            case TOK_RPAREN:
            case TOK_LAB:
            case TOK_RAB:
            case TOK_NL:
                return;
                break;
            default:
                // For every regular token, append it as text arguments to the parent SIMPLE_COMMAND
                simple_append_word(context->root->type, g_tok_buffer + g_tok_list[g_tok_index].offset);
                break;
            }
        }
        else
        {
            // Switch on the current token's type
            switch (g_tok_list[g_tok_index].type)
            {
            case TOK_IF:
                // Add the new node to the supertree
                inner_context.cur_node = inner_context.root = insert_node(SUBSHELL_COMMAND, context);

                // Call shell_inner() recursively, passing the subtree context
                shell_inner(&inner_context);
                break;
            case TOK_THEN:
                // If the root (parent) node type is not IF_COMMAND
                if(context->root->type != IF_COMMAND)
                {
                    fprintf(stderr, "%ull: Syntax error: unexpected `then'\n", line_num);
                }
                break;
            case TOK_FI:

                break;
            case TOK_ELSE:

                break;
            case TOK_WHILE:

                break;
            case TOK_UNTIL:

                break;
            case TOK_DO:

                break;
            case TOK_DONE:

                break;
            case TOK_WORD:
                // If the parent is a simple command, append the word to the command
                if(context->root->type == SIMPLE_COMMAND)
                {
                    simple_append_word(context->root->type, g_tok_buffer + g_tok_list[g_tok_index].offset);
                }
                else
                {

                }
                break;
            case TOK_SC:

                break;
            case TOK_PIPE:

                break;
            case TOK_LPAREN:

                break;
            case TOK_RPAREN:

                break;
            case TOK_LAB:

                break;
            case TOK_RAB:

                break;
            case TOK_NL:

                break;
            case TOK_COL:

                break;
            }
        }
    }
    while(!getTok());
}

void shell()
{
    // Create an initial context; this is the root of all trees in the recursive structure
    tree_context initial_context;
    initial_context.cur_node = initial_context.root = NULL;

    // Call shell_inner
    shell_inner(&initial_context);
}
