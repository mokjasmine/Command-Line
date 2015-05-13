#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */ 


char specialChars[7] = {';', '|', '&', '(', ')', '<', '>'};
char*** readListArray;
char*** writeListArray;
int readListArray_size = 0;
int writeListArray_size = 0;


command_stream_t
make_command_stream (int (*get_next_byte) (void *),
         void *get_next_byte_argument)
{
  char *commandString = malloc(sizeof(char));
  if (commandString == NULL)
    perror("Could not allocate memory.");
  char inputChar = ' ';
  int numChars = 0;
  while (1)
  { 
    inputChar = get_next_byte(get_next_byte_argument);

    if (inputChar == EOF)
      break;

    numChars++;

    commandString = realloc(commandString,(numChars)*sizeof(char));
    if (commandString == NULL)
      perror("Could not allocate memory."); 

    commandString[numChars-1] = inputChar;
  }

  if (numChars == 0)
    return NULL;

  int i=0;
  for (i = numChars; i >=0; i--)
  {
    if (commandString[i] == ' ' || commandString[i] == '\v' || commandString[i] == '\r' || commandString[i] == '\t')
    {
      commandString[i] = '\0';
    }
    else
      break;
  }

  char* currentChar = malloc(numChars*sizeof(char));
  int s;
  i = 0;
  int opencount=0;
  int closecount=0;
  bool deleteSpace = false;
  for(s=0; s<numChars; s++)
  {
    if (commandString[s] != ' ' && commandString[s] != '\v' && commandString[s] != '\r' && commandString[s] != '\t')
    { 
      if (commandString[s] == '#')
      { 
        while(commandString[s] != '\n')
        {
          s++;
        }
        continue;
      }
      currentChar[i] = commandString[s];
      i++;
      if (commandString[s] == '\n')
        deleteSpace = true;
      else
        deleteSpace = false;
    }
    else if (deleteSpace)
    {
      continue;
    }
    else
    {
      if (commandString[s+1] == '\0')
        break;
      currentChar[i] = commandString[s];
      i++;
      deleteSpace = true;
    }
  }
  currentChar[i] = '\0';


  char *c2 = currentChar;
  while (*c2 != '\0')
  {
    if(*c2=='(')
    {
      opencount++;
      c2++;
    }
    else if(*c2==')')
    {
      closecount++;
      c2++;
    }
    else if(*c2=='\n')
    {
      if (*(c2+1) != '\0' && *(c2+1) == '\n')
      {
        if(opencount!=closecount)
        {
          fprintf(stderr, "error");
          exit(1);
        }
        opencount=0;
        closecount=0;
        c2+=2;
      }
      else
        c2++;
    }
    else
      c2++;
  }

  if(opencount!=closecount)
        {
          fprintf(stderr, "error");
          exit(1);
        }


  int numNewlines;
  for (i = 0; currentChar[i] != '\0'; i++)
  {
    if (currentChar[i] == '\n')
      numNewlines++;
  }

  readListArray = malloc(numNewlines*sizeof(char**));
  writeListArray = malloc(numNewlines*sizeof(char**));
  int readList_size = 0;
  int writeList_size = 0;

  
  readListArray[0] = malloc(numNewlines*sizeof(char*));
  writeListArray[0] = malloc(numNewlines*sizeof(char*));
  

  Stack commandStack;
  commandStack.size = 0;
  Stack operatorStack;
  operatorStack.size = 0;
  
  struct command *prevCommand = NULL;
  char* c;
  struct command *commandArray = malloc(numChars*sizeof(struct command));
  char** wordArray = malloc(numChars*sizeof(char*));
  int numWords = 0;
  int numCommands = 0;
  int numLine = 1;

  command_stream_t stream = malloc(sizeof(command_stream));
  stream->head = NULL;
  stream->cursor = NULL;
  stream->tail = NULL;
  currentChar[i] = '\0';


  while(*currentChar != '\0')
  {

    if (!isalpha(*currentChar) && !isdigit(*currentChar) && *currentChar != '!' && *currentChar != '%' && *currentChar != '+' && *currentChar != ','
        && *currentChar != '-' && *currentChar != '.' && *currentChar != '/' && *currentChar != ':' && *currentChar != '@' && *currentChar != '^' &&
        *currentChar != '_' && *currentChar != ';' && *currentChar != '|' && *currentChar != '&' && *currentChar != '(' && *currentChar != ')' &&
        *currentChar != '<' && *currentChar != '>' && *currentChar != ' ' && *currentChar != '\t' && *currentChar != '\n' && *currentChar != '\r' && *currentChar != '\v')
      {
        fprintf(stderr, "error");
        exit(1);
      }

    if (*currentChar == ' ' || *currentChar == '\v' || *currentChar == '\t' || *currentChar == '\r')
    { 
      currentChar++;
      continue;
    }

    

    bool newTree = false;
    if (*currentChar == '\n')
    {
      numLine++;
      if (prevCommand->type != SIMPLE_COMMAND)
      {
        currentChar++;
        continue;
      }

      if (*(currentChar+1) == '\n')
      {
        if(prevCommand->type == AND_COMMAND || prevCommand->type == SEQUENCE_COMMAND || prevCommand->type == OR_COMMAND || 
            prevCommand->type == PIPE_COMMAND)
        {
          fprintf(stderr, "error");
          exit(1);
        }
        numLine++;
        
        prevCommand->u.word[prevCommand->numWords] = NULL;

        readListArray[readListArray_size][readList_size] = NULL;
        writeListArray[writeListArray_size][writeList_size] = NULL;
        readListArray_size++;
        writeListArray_size++;
        readList_size = 0;
        writeList_size = 0;
        
        readListArray[readListArray_size] = malloc(numNewlines*sizeof(char*));
        writeListArray[writeListArray_size] = malloc(numNewlines*sizeof(char*));
        

        while(operatorStack.size !=0)
        {
          operatorStack.top->command->u.command[1] = commandStack.top->command;
          pop(&commandStack);
          operatorStack.top->command->u.command[0] = commandStack.top->command;
          pop(&commandStack);
          push(&commandStack, top(&operatorStack)->command);
          pop(&operatorStack);
        }
        if (stream->head == NULL)
        {
          stream->head = commandStack.top;
          stream->cursor = stream->head;
          stream->tail = commandStack.top;
        } 
        else
        {
          stream->tail->next = commandStack.top;
          stream->tail = commandStack.top;
        }

        pop(&commandStack);
        newTree = true;
        prevCommand = NULL;
        currentChar+=2;
      }
      
      else
      {
        *currentChar = ';';
//        currentChar++;
//        continue;
      }

    }

    if(newTree)
      continue;

      //if AND_COMMAND
    if (*currentChar == '&')
    {
      if(prevCommand == NULL || prevCommand->type == AND_COMMAND || prevCommand->type == OR_COMMAND ||
      prevCommand->type == PIPE_COMMAND || prevCommand->type == SEQUENCE_COMMAND || *(currentChar-1)=='\n') 
      {
        fprintf(stderr, "error");
        exit(1);
      }

      if(prevCommand->type == SIMPLE_COMMAND)
      {
        prevCommand->u.word[prevCommand->numWords] = NULL;
      }

      if(*(currentChar+1) == '&')
      {
        struct command and_command;
        and_command.type = AND_COMMAND;
        and_command.input = NULL;
        and_command.output = NULL;

        if (operatorStack.size == 0)
        {
//          commandArray = realloc(commandArray, (numCommands+1)*sizeof(struct command));
          commandArray[numCommands] = and_command;
          push(&operatorStack, &commandArray[numCommands]);
          numCommands++;
        }
        else
        {
          while(operatorStack.size != 0 && (top(&operatorStack)->command->type != SUBSHELL_COMMAND))
          {
            if (top(&operatorStack)->command->type == OR_COMMAND ||
                top(&operatorStack)->command->type == PIPE_COMMAND ||
                top(&operatorStack)->command->type == AND_COMMAND)
            {
              top(&operatorStack)->command->u.command[1] = top(&commandStack)->command;
              pop(&commandStack);
              top(&operatorStack)->command->u.command[0] = top(&commandStack)->command;
              pop(&commandStack);
              push(&commandStack, top(&operatorStack)->command);
              pop(&operatorStack);
            }
            else if (top(&operatorStack)->command->type == SEQUENCE_COMMAND)
                break;
          }

//          commandArray = realloc(commandArray, (numCommands+1)*sizeof(struct command));
          commandArray[numCommands] = and_command;
          push(&operatorStack, &commandArray[numCommands]);
          numCommands++;
        }

        currentChar+=2;
      }
    }
    //if input or output
    else if (*currentChar == '<' || *currentChar == '>')
    {
      if(prevCommand == NULL || prevCommand->type == AND_COMMAND || prevCommand->type == OR_COMMAND ||
      prevCommand->type == PIPE_COMMAND || prevCommand->type == SEQUENCE_COMMAND || *(currentChar-1)=='\n' || *(currentChar+1) == '<' ||
      *(currentChar+1) == '>') 
      {
        fprintf(stderr, "error");
        exit(1);
      }
      if(*currentChar == '<')
      {
        if(prevCommand->output != NULL)
        {
          fprintf(stderr, "error");
          exit(1);
        }
      }
      
      int wordSize = 0;
      int numSpaces = 0;
      char *currentWord1 = malloc(sizeof(char));
      char* c1;

      if (*(currentChar+1) == ' ' || *(currentChar+1) == '\v' || *(currentChar+1) == '\r' || *(currentChar+1) == '\t')
      {
        c1 = currentChar+2;
        numSpaces++;
      }
      else
        c1 = currentChar+1;

      for (c = c1;(*c != '\0' && *c != ' ' && *c != '\v' && *c != '\r' && *c != '\t' && *c!= '\t'); c++)
      {
        bool isSpecial = false;
        for (i = 0; i < 7; i++)
        {
          if (*c == specialChars[i])
            isSpecial = true;
        }
        if (isSpecial)
          break;
        wordSize++;
        currentWord1 = realloc(currentWord1,wordSize*sizeof(char));
        currentWord1[wordSize-1] = *c;
      }

      currentWord1 = realloc(currentWord1,(1+wordSize)*sizeof(char));
      currentWord1[wordSize] = '\0';

      wordArray[numWords] = currentWord1;
      numWords++;

      if (*currentChar == '<')
      {
        prevCommand->input = wordArray[numWords-1];
        readListArray[readListArray_size][readList_size] = wordArray[numWords-1];
        readList_size++;
      }
      else
      {
        prevCommand->output = wordArray[numWords-1];
        writeListArray[writeListArray_size][writeList_size] = wordArray[numWords-1];
        writeList_size++;
      }
      currentChar += (1+numSpaces+wordSize);

    }
    //if SEQUENCE_COMMAND
    else if (*currentChar == ';')
    {
      if(prevCommand == NULL || prevCommand->type == AND_COMMAND || prevCommand->type == OR_COMMAND ||
      prevCommand->type == PIPE_COMMAND || prevCommand->type == SEQUENCE_COMMAND || *(currentChar-1)=='\n') 
      {
        fprintf(stderr, "error");
        exit(1);
      }

      if (prevCommand->type == SIMPLE_COMMAND)
      {
        prevCommand->u.word[prevCommand->numWords] = NULL;
      }

      struct command sequence_command;
      sequence_command.type = SEQUENCE_COMMAND;
      sequence_command.input = NULL;
      sequence_command.output = NULL;

      if (operatorStack.size == 0)
      {
//          commandArray = realloc(commandArray, (numCommands+1)*sizeof(struct command));
        commandArray[numCommands] = sequence_command;
        push(&operatorStack, &commandArray[numCommands]);
        numCommands++;
      }
      else
      {
        while(operatorStack.size != 0 && (top(&operatorStack)->command->type != SUBSHELL_COMMAND))
        {
          if (top(&operatorStack)->command->type == OR_COMMAND ||
             top(&operatorStack)->command->type == PIPE_COMMAND ||
              top(&operatorStack)->command->type == AND_COMMAND ||
              top(&operatorStack)->command->type == SEQUENCE_COMMAND)
          {
            top(&operatorStack)->command->u.command[1] = top(&commandStack)->command;
            pop(&commandStack);
            top(&operatorStack)->command->u.command[0] = top(&commandStack)->command;
            pop(&commandStack);
            push(&commandStack, top(&operatorStack)->command);
            pop(&operatorStack);
          }
        }

//          commandArray = realloc(commandArray, (numCommands+1)*sizeof(struct command));
        commandArray[numCommands] = sequence_command;
        push(&operatorStack, &commandArray[numCommands]);
        numCommands++;
      }

      currentChar++;
      
    }
    else if (*currentChar == '|')
      {
        if(prevCommand == NULL || prevCommand->type == AND_COMMAND || prevCommand->type == OR_COMMAND ||
        prevCommand->type == PIPE_COMMAND || prevCommand->type == SEQUENCE_COMMAND || *(currentChar-1)=='\n') 
        {
          fprintf(stderr, "error");
          exit(1);
        }
        //if OR_COMMAND
        if (*(currentChar+1) == '|')
        {
          if (prevCommand->type == SIMPLE_COMMAND)
          {
            prevCommand->u.word[prevCommand->numWords] = NULL;
          }
          struct command or_command;
          or_command.type = OR_COMMAND;
          or_command.input = NULL;
          or_command.output = NULL;

          if (operatorStack.size == 0)
          {
//          commandArray = realloc(commandArray, (numCommands+1)*sizeof(struct command));
            commandArray[numCommands] = or_command;
            push(&operatorStack, &commandArray[numCommands]);
            numCommands++;
          }
          else
          {
            while(operatorStack.size != 0 && (top(&operatorStack)->command->type != SUBSHELL_COMMAND))
            {
              if (top(&operatorStack)->command->type == OR_COMMAND ||
                 top(&operatorStack)->command->type == PIPE_COMMAND ||
                  top(&operatorStack)->command->type == AND_COMMAND)
              {
                top(&operatorStack)->command->u.command[1] = top(&commandStack)->command;
                pop(&commandStack);
                top(&operatorStack)->command->u.command[0] = top(&commandStack)->command;
                pop(&commandStack);
                push(&commandStack, top(&operatorStack)->command);
                pop(&operatorStack);
              }
              else if (top(&operatorStack)->command->type == SEQUENCE_COMMAND)
                break;
            }

//          commandArray = realloc(commandArray, (numCommands+1)*sizeof(struct command));
            commandArray[numCommands] = or_command;
            push(&operatorStack, &commandArray[numCommands]);
            numCommands++;
          }

          currentChar+=2;
        }
    
        //if PIPE_COMMAND
        else
        {
          struct command pipe_command;
          pipe_command.type = PIPE_COMMAND;
          pipe_command.input = NULL;
          pipe_command.output = NULL;

          if (operatorStack.size == 0)
          {
//          commandArray = realloc(commandArray, (numCommands+1)*sizeof(struct command));
            commandArray[numCommands] = pipe_command;
            push(&operatorStack, &commandArray[numCommands]);
            numCommands++;
          }
          else
          {
            while(operatorStack.size != 0 && (top(&operatorStack)->command->type != SUBSHELL_COMMAND))
            {
              if (top(&operatorStack)->command->type == PIPE_COMMAND)
              {
                top(&operatorStack)->command->u.command[1] = top(&commandStack)->command;
                pop(&commandStack);
                top(&operatorStack)->command->u.command[0] = top(&commandStack)->command;
                pop(&commandStack);
                push(&commandStack, top(&operatorStack)->command);
                pop(&operatorStack);
              }
              else
                break;
            }

//          commandArray = realloc(commandArray, (numCommands+1)*sizeof(struct command));
            commandArray[numCommands] = pipe_command;
            push(&operatorStack, &commandArray[numCommands]);
            numCommands++; 
          }

          currentChar++;
        }
      }
    //if SUBSHELL_COMMAND  
    else if (*currentChar == '(')
    {


      /*
      if(prevCommand->type == SUBSHELL_COMMAND)
      {
        fprintf(stderr, numLine+":");
        exit(1);
      }
      */
      for (c = currentChar; *c != ')'; c++)
      {
        if (*c == '\0')
          perror("Mismatched parenthesis");
      } 
      struct command subshell_command;
      subshell_command.type = SUBSHELL_COMMAND;
      subshell_command.status = -1;
      subshell_command.input = NULL;
      subshell_command.output = NULL;

      commandArray[numCommands] = subshell_command;
      push(&operatorStack, &commandArray[numCommands]);
      numCommands++;

      currentChar++;
    }
    else if (*currentChar == ')')
    {
      if(prevCommand->type != SIMPLE_COMMAND)
      {
        fprintf(stderr, "error");
        exit(1);
      }
      while(1)
      {
        if (operatorStack.top->command->type == SUBSHELL_COMMAND)
        {
          operatorStack.top->command->u.subshell_command = commandStack.top->command;
          pop(&commandStack);
          push(&commandStack, operatorStack.top->command);
          pop(&operatorStack);
          break;
        }
        else 
        {
            
          operatorStack.top->command->u.command[1] = commandStack.top->command;
          pop(&commandStack);
          operatorStack.top->command->u.command[0] = commandStack.top->command;
          pop(&commandStack);
          push(&commandStack, top(&operatorStack)->command);
          pop(&operatorStack);
        }
      }
      currentChar++;
    }
    //if SIMPLE_COMMAND
    else
    {
      int wordSize = 0;
      char *currentWord = malloc(sizeof(char));
      for (c = currentChar; (*c != ' ') && (*c != '\v') && (*c != '\r') && (*c != '\t') && (*c != '\n') && (*c != '\0'); c++)
      {
        int i;
        bool isSpecial = false;
        for (i = 0; i < 7; i++)
        {
          if (*c == specialChars[i])
            isSpecial = true;
        }
        if (isSpecial)
          break;

        wordSize++;
        currentWord = realloc(currentWord,wordSize*sizeof(char));
        currentWord[wordSize-1] = *c;
      }

      currentWord = realloc(currentWord,(1+wordSize)*sizeof(char));
      currentWord[wordSize] = '\0';

      wordArray[numWords] = currentWord;
      numWords++;

      if ((prevCommand != NULL) && (prevCommand->type == SIMPLE_COMMAND))
      {
        prevCommand->u.word = realloc(prevCommand->u.word, (prevCommand->numWords+1)*sizeof(char*));
        prevCommand->u.word[prevCommand->numWords] = wordArray[numWords-1];
        prevCommand->u.word[prevCommand->numWords+1] = NULL;

        prevCommand->numWords++;
        readListArray[readListArray_size][readList_size] = wordArray[numWords-1];
        readList_size++;

      }
      else
      {
        struct command simple_command;
        simple_command.type = SIMPLE_COMMAND;
        simple_command.status = -1;
        simple_command.input = NULL;
        simple_command.output = NULL;
        simple_command.u.word = malloc(sizeof(char*));
        simple_command.u.word[0] = wordArray[numWords-1];
        simple_command.numWords = 1;

//        commandArray = realloc(commandArray, (numCommands+1)*sizeof(struct command));
        commandArray[numCommands] = simple_command;
        push(&commandStack, &commandArray[numCommands]);
        numCommands++;
      }

      currentChar+= wordSize;

    }  

    prevCommand = &commandArray[numCommands-1];
    

  }

  wordArray[numWords] = NULL;

  if (prevCommand->type != SIMPLE_COMMAND)
  {
    fprintf(stderr, "error");
    exit(1);
  }

  prevCommand->u.word[prevCommand->numWords] = NULL;

  readListArray[readListArray_size][readList_size] = NULL;
  writeListArray[writeListArray_size][writeList_size] = NULL;
  readListArray_size++;
  writeListArray_size++;

  while(operatorStack.size !=0)
        {
          operatorStack.top->command->u.command[1] = commandStack.top->command;
          pop(&commandStack);
          operatorStack.top->command->u.command[0] = commandStack.top->command;
          pop(&commandStack);
          push(&commandStack, top(&operatorStack)->command);
          pop(&operatorStack);
        }
        if (stream->head == NULL)
        {
          stream->head = commandStack.top;
          stream->cursor = stream->head;
          stream->tail = commandStack.top;
        } 
        else
        {
          stream->tail->next = commandStack.top;
          stream->tail = commandStack.top;
        }

        pop(&commandStack);
        
/*
  operatorStack.top->command->u.command[1] = commandStack.top->command;
  pop(&commandStack);
  operatorStack.top->command->u.command[0] = commandStack.top->command;
  pop(&commandStack);
*/

  return stream;


  


  /*for testing
  Stack s1;
  command_t c1 = malloc(sizeof(struct command));
  c1->status = 10;
  push(&s1, c1);
  command_t c2 = malloc(sizeof(struct command));
  c2->status = 20;
  push(&s1, c2);
  pop(&s1);
  int i = top(&s1)->command->status;
  

  command_stream *test = malloc(sizeof(command_stream));
  test->head = NULL;
  test->tail = NULL;
  test->cursor = NULL;
  return test;
  */
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functionons defined in the GNU C Library.  */


}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  if (s->cursor == NULL)
    return NULL;

  struct commandNode *temp = s->cursor;
  s->cursor = s->cursor->next;
  return temp->command;
}