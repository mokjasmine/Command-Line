#include <stdio.h>
#include "command.h"
#include "command-internals.h"


void push(Stack *S, command_t newCommand)
{
    struct commandNode *newNode = malloc(sizeof(commandNode));
    newNode->command = newCommand;
    if (S->size == 0)
        newNode->next = NULL;
    else
        newNode->next = S->top;
    S->top = newNode;
    S->size = S->size + 1;
}

struct commandNode* top(Stack *S)
{
    return (S->top);
}

void pop(Stack *S)
{
    S->top = (S->top)->next;
    S->size = S->size - 1;
}

bool isEmpty(Stack *S)
{
    return (S->size == 0) //return true if empty
}