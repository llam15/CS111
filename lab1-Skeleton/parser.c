#include "alloc.h"
#include "command-internals.h"
#include "command.h"
#include "parser.h"
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#define DEFAULT_WORD_NUM (4)

static Token_t* g_tok_list;
static uint64_t g_tok_index;
static uint64_t g_tok_list_len;
static char* g_tok_buffer;

// A tree. Used to store the head of a recursively-traversed section of the command tree
typedef struct
{
    command_t root;
    command_t cur_node;
} tree_context;

// Currently processing line number
static uint64_t line_num;

// Parse a token list and store the generated command tree into parsed_commands->command_tree
void parse(Token_t* tok_list, char* tok_buffer, uint64_t tok_list_len, command_t* ret_tree)
{
    g_tok_buffer = tok_buffer;
    g_tok_index = 0;
    g_tok_list = tok_list;
    g_tok_list_len = tok_list_len;
    *ret_tree = shell();
}

// Moves the read index forward, and returns true on end of token list
bool getTok(void)
{
    if (g_tok_index == g_tok_list_len)
        return true;
    g_tok_index++;
    return false;
}

// Inserts a child command into the ->u.commands list in the parent.
// Keeps track of position in the 'commands' list by checking for NULL
// Returns position inserted at, or negative on error
// Note: Make sure that parent and child are both malloc'd
int8_t insert_child(command_t parent, command_t child)
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
    {
        fprintf(stderr, "%"PRIu64": Syntax Error: Token nesting not supported for command num %d\n", line_num, parent->type);
        return -1;
    }

    // Insert into the first NULL space; return the position inserted into
    uint8_t child_index;
    for(child_index = 0; child_index < num_children; child_index++)
    {
        if(parent->u.command[child_index] == NULL)
        {
            parent->u.command[child_index] = child;
            return child_index;
        }
    }

    fprintf(stderr, "%"PRIu64": Syntax Error: Too many child tokens following command num %d\n", line_num, parent->type);
    return -2;
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
            ret->u.word[i] = NULL;
        }
    }
    return ret;
}

// Attach another word to the end of the words pointer list on a simple_command
void simple_append_word(command_t simple_command, char* word)
{
    if(simple_command->type != SIMPLE_COMMAND)
    {
        fprintf(stderr, "%"PRIu64": Internal error: attempt to append %s to non-simple command\n", line_num, word);
        return;
    }
    // If the word index overruns the buffer, attempt to reallocate
    if(simple_command->word_index == simple_command->n_words)
    {
        // Expand the buffer exponentially by two
        simple_command->n_words *= 2;
        // Checked-realloc (allocate one extra spot for a terminating null pointer)
        simple_command->u.word = (char**)checked_realloc(simple_command->u.word, sizeof(char*) * (simple_command->n_words + 1));

        // Initialize newly allocated pointers to NULL
        int i;
        for(i = simple_command->word_index + 1; i < (simple_command->n_words + 1); i++)
        {
            simple_command->u.word[i] = NULL;
        }
    }
    // Put the word at the next position
    simple_command->u.word[simple_command->word_index++] = word;
}

// Note: returns the newly created node
uint8_t insert_node(command_type type, tree_context * context, command_t* ret_command)
{
    int8_t rval = 0;

    command_t node = construct_node(type);

    if (context->root == NULL)
        context->root = node;

    if(context->cur_node != NULL)
    {
        rval = insert_child(context->cur_node, node);
        if(rval < 0)
            exit(rval);

    }

    context->cur_node = node;
    *ret_command = node;
    return rval;
}

void shell_inner(tree_context * context)
{
    printf("shell_inner: tree_context =\t%"PRIu64"\t%"PRIu64"\n", (uint64_t)context, (uint64_t)context->root);
    static uint8_t insertion_index = 0;
    while(!getTok())
    {
        // Create a new tree context to hold the branch
        tree_context inner_context;
        command_t temp_root = NULL;

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
                simple_append_word(context->root, g_tok_buffer + g_tok_list[g_tok_index].offset);
                break;
            }
        }
        else
        {
            // Switch on the current token's type
            switch (g_tok_list[g_tok_index].type)
            {
            case TOK_IF:
                // Add the new node to the supertree; maintain insertion index
                insertion_index = insert_node(SUBSHELL_COMMAND, context, &inner_context.root);
                inner_context.cur_node = inner_context.root;

                // Call shell_inner() recursively, passing the subtree context
                shell_inner(&inner_context);
                break;
            case TOK_THEN:
                // If the root (parent) node type is not IF_COMMAND, or if a TOK_THEN has already been seen
                if(context->root->type != IF_COMMAND || insertion_index != 0)
                {
                    fprintf(stderr, "%"PRIu64": Syntax error: unexpected `then'\n", line_num);
                    exit(-1);
                }
                break;
            case TOK_FI:
                if(context->root->type != IF_COMMAND || insertion_index == 0)
                {
                    fprintf(stderr, "%"PRIu64": Syntax error: unexpected `fi'\n", line_num);
                    exit(-1);
                }
                // Pop!
                return;
                break;
            case TOK_ELSE:
                // If the root (parent) node type is not IF_COMMAND, or if a TOK_THEN has already been seen
                if(context->root->type != IF_COMMAND || insertion_index != 1)
                {
                    fprintf(stderr, "%"PRIu64": Syntax error: unexpected `then'\n", line_num);
                    exit(-1);
                }
                break;
            case TOK_WHILE:
                // Add the new node to the supertree; maintain insertion index
                insertion_index = insert_node(WHILE_COMMAND, context, &inner_context.root);
                inner_context.cur_node = inner_context.root;

                // Call shell_inner() recursively, passing the subtree context
                shell_inner(&inner_context);
                break;
            case TOK_UNTIL:
                // Add the new node to the supertree; maintain insertion index
                insertion_index = insert_node(UNTIL_COMMAND, context, &inner_context.root);
                inner_context.cur_node = inner_context.root;

                // Call shell_inner() recursively, passing the subtree context
                shell_inner(&inner_context);
                break;
            case TOK_DO:
                if(context->root->type != WHILE_COMMAND || context->root->type != UNTIL_COMMAND || insertion_index != 0)
                {
                    fprintf(stderr, "%"PRIu64": Syntax error: unexpected `do'\n", line_num);
                }
                break;
            case TOK_DONE:
                if(context->root->type != WHILE_COMMAND || context->root->type != UNTIL_COMMAND || insertion_index != 1)
                {
                    fprintf(stderr, "%"PRIu64": Syntax error: unexpected `done'\n", line_num);
                    exit(-1);
                }
                // Pop!
                return;
                break;
            case TOK_WORD:
                // Add the new node to the supertree; maintain insertion index
                insertion_index = insert_node(SIMPLE_COMMAND, context, &inner_context.root);
                inner_context.cur_node = inner_context.root;

                // Call shell_inner() recursively, passing the subtree context
                shell_inner(&inner_context);
                break;
            case TOK_SC:
            case TOK_PIPE:
            case TOK_NL:
                // Store the old root temporarily so tree can be reordered
                temp_root = context->root;

                context->root = context->cur_node = NULL;
                char* str_type = "Unknown linking";
                if(g_tok_list[g_tok_index].type == TOK_SC)
                {
                    str_type = "Sequence";
                    insertion_index = insert_node(SEQUENCE_COMMAND, context, &inner_context.root);
                }
                if(g_tok_list[g_tok_index].type == TOK_PIPE)
                {
                    str_type = "Pipe";
                    insertion_index = insert_node(PIPE_COMMAND, context, &inner_context.root);
                }
                if(g_tok_list[g_tok_index].type == TOK_NL)
                {
                    str_type = "New line";
                    insertion_index = insert_node(SEQUENCE_COMMAND, context, &inner_context.root);
                }
                inner_context.cur_node = inner_context.root;

                // Insert the whole tree that used to be located at context->root (now at temp_root) under the new sequence command. Should end up being inserted at the zeroth index.
                if(insert_child(inner_context.root, temp_root) != 0)
                {
                    fprintf(stderr, "%"PRIu64": Internal error: %s token has incorrect number of children.\n", line_num, str_type);
                }

                // Call shell_inner() recursively, passing the subtree context
                shell_inner(&inner_context);
                break;
            case TOK_LPAREN:

                break;
            case TOK_RPAREN:

                break;
            case TOK_LAB:

                break;
            case TOK_RAB:

                break;
            case TOK_COL:

                break;
            }
        }
    }

}

command_t shell()
{
    // Create an initial context; this is the root of all trees in the recursive structure
    tree_context* initial_context = (tree_context*)checked_malloc(sizeof(tree_context));
    initial_context->cur_node = initial_context->root = construct_node(SUBSHELL_COMMAND);

    // Call shell_inner
    shell_inner(initial_context);

    command_t ret = initial_context->root->u.command[0];

    free(initial_context->root);

    return ret;
}
