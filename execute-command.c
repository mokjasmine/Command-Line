// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


typedef struct GraphNode
{
	command_t command;
	struct GraphNode** before;
	pid_t pid;
	struct GraphNode* next;
	struct GraphNode* prev;
}GraphNode;


/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
int execute_command_type(command_t c);

int
command_status (command_t c)
{
  return c->status;
}




void
execute_command (command_t c, bool time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  execute_command_type(c);
  //error (1, 0, "command execution not yet implemented");
}

int execute_command_type(command_t c)
{
	switch(c->type)
	{
		case AND_COMMAND:
		{
			execute_command_type(c->u.command[0]);
			if(command_status(c->u.command[0]) == 0) //if first command successful
			{
				execute_command_type(c->u.command[1]);
				c->status = command_status(c->u.command[1]);
			}
			else	
				c->status = command_status(c->u.command[0]);
			break;
		}
		case OR_COMMAND:
		{
			execute_command_type(c->u.command[0]);
			if(command_status(c->u.command[0]) != 0) //if first command unsuccessful
			{
				execute_command_type(c->u.command[1]);
				c->status = command_status(c->u.command[1]);
			}
			else
				c->status = command_status(c->u.command[0]);
			break;
		}
		case SEQUENCE_COMMAND:
		{
			execute_command_type(c->u.command[0]);
			execute_command_type(c->u.command[1]);
			// what to set as c->status?
			break;
		}
		case PIPE_COMMAND:
		{
			int status;
			int fd[2];
			if (pipe(fd) == -1)
			{
				fprintf(stderr, "error\n");
			  	exit(1);
			}
			int firstPid = fork();
			if (firstPid == 0)
			{
				close(fd[0]);
				if(dup2(fd[1],STDOUT_FILENO) == -1)
				{
					fprintf(stderr, "error\n");
					exit(1);
				}

				execute_command_type(c->u.command[0]);

				close(fd[1]); 
				exit(command_status(c->u.command[0])); 
			}
			else if (firstPid > 0)
			{
				waitpid(firstPid, &status, 0);
			if (status!=0)
			{
			  	c->status = command_status(c->u.command[0]);
			}
			else
			{
			  close(fd[1]);
			  if (dup2(fd[0], STDIN_FILENO) == -1)
			  {
			  	fprintf(stderr, "error\n");
					exit(1);
			  }
			  execute_command_type(c->u.command[1]);
			  close(fd[0]);
			  c->status = command_status(c->u.command[1]);
			}
			}
			else
			{
				fprintf(stderr, "error\n");
				exit(1);
			}      
			break;

		}
		case SIMPLE_COMMAND:
		{
			int pid = fork();
			if(pid == 0)
			{
				if (c->input != NULL)
				{
					int fd_in = open(c->input, O_RDONLY);
					if (fd_in < 0)
						return -1;
					dup2(fd_in, 0);
					close(fd_in);
				}

				if (c->output != NULL)
				{
					int fd_out = open(c->output, O_CREAT | O_TRUNC | O_WRONLY, 0644);
					if (fd_out < 0)
						return -1;
					dup2(fd_out, 1);
					close(fd_out);
				}	
				if (execvp(c->u.word[0], c->u.word) != 0)
				{
					fprintf(stderr, "error\n");
					exit(1);
				}
//				close(0);
//				close(1);
			}
			else if(pid > 0)
			{
				int status;
				waitpid(pid, &status, 0);
				int exitstatus = WEXITSTATUS(status);
				c->status = exitstatus;
			}
			break;
		}
		case SUBSHELL_COMMAND:
		{
			execute_command_type(c->u.subshell_command); //finish
			break;
		}
	}
	return c->status;
}

dependencyGraph *createGraph(command_stream_t stream)
{
	int i = 0;
	int j = 0;
	int k=0;
	int l = 0;
	bool dependent;
	bool added;
	commandNode *temp = stream->head;
	commandNode *temp2;

	struct dependencyGraph *dependencygraph = malloc(2*sizeof(GraphNode_t));
	dependencygraph->no_dependencies = NULL;
	dependencygraph->dependencies = NULL;
	struct GraphNode *curr;
	
//	for (i=0; i<readListArray_size; i++) // check readlist for dependencies on writes
	while(1)
	{
		if (temp == NULL)
			break;
		int before_size = 0;
		temp->graphnode = malloc(sizeof(GraphNode));
		temp->graphnode->before = malloc(readListArray_size*sizeof(GraphNode_t));
		temp->graphnode->command = temp->command;
		temp->graphnode->pid = -1;
		dependent = false;
		added = false;

		j = 0;
		
		while(readListArray[i][j] != NULL) 
		{
			for (k = 0; k < i; k++)
			{
				while (writeListArray[k][l] != NULL)
				{
					if (strcmp(readListArray[i][j],writeListArray[k][l]) == 0)
					{
						temp2 = stream->head;
						int m = 0;
						while(m < k)
						{
							temp2 = temp2->next;
							m++;
						}

						temp->graphnode->before[before_size] = temp2->graphnode;
						before_size++;
						if (!added)
						{
//							curr = (GraphNode *)malloc(sizeof(GraphNode));
							curr = temp->graphnode;
							curr->next = dependencygraph->dependencies;
							if (curr->next != NULL)
								curr->next->prev = curr;
							curr->prev = NULL;
							dependencygraph->dependencies = curr;
							dependent=true;
							added = true;
						}
					}
					l++;
				}
				l = 0;
			}
			j++;
		}
		j = 0;
		l = 0;
		while(writeListArray[i][j] != NULL) 
		{
			for (k = 0; k < i; k++)
			{
				while (readListArray[k][l] != NULL)
				{
					if (strcmp(writeListArray[i][j],readListArray[k][l]) == 0 
						|| (writeListArray[k][l] != NULL && strcmp(writeListArray[i][j],writeListArray[k][l]) == 0))
					{
						temp2 = stream->head;
						int m = 0;
						while(m < k)
						{
							temp2 = temp2->next;
							m++;
						}
						temp->graphnode->before[before_size] = temp2->graphnode;
						before_size++;
						if (!added)
						{
//							curr = (GraphNode *)malloc(sizeof(GraphNode));
							curr = temp->graphnode;
							curr->next = dependencygraph->dependencies;
							if (curr->next != NULL)
								curr->next->prev = curr;
							curr->prev = NULL;
							dependencygraph->dependencies = curr;
							dependent=true;
							added = true;
						}
					}
					l++;
				}
				l = 0;
			}
			j++;
		}
		if(!dependent)
		{
			curr = (GraphNode *)malloc(sizeof(GraphNode));
			curr = temp->graphnode;
			curr->next = dependencygraph->no_dependencies;
			if (curr->next != NULL)
				curr->next->prev = curr;
			dependencygraph->no_dependencies = curr;

		}
		j = 0;
		l = 0;
		temp = temp->next;
		i++;
		before_size = 0;
	}


	/*temp = stream->head;
	temp2 = stream->head;
	i = 0;
	j = 0;
	k = 0;
	l = 0;

	while(1)
	{
		if (temp == NULL)
			break;
		int before_size = 0;
	}*/
	return dependencygraph;
}



void executeGraph(dependencyGraph *graph)
{
	executeNoDependencies(graph->no_dependencies);
	executeDependencies(graph->dependencies);
	
	GraphNode_t tmp = graph->no_dependencies;
	int status;
	while (tmp != NULL)
	{
		if (tmp->pid > 0)
		{
			waitpid(tmp->pid, &status, 0);
		}
		tmp = tmp->next;
	}

	tmp = graph->dependencies;
	while(tmp != NULL)
	{
		if (tmp->pid > 0)
		{
			waitpid(tmp->pid, &status, 0);
		}
		tmp = tmp->next;
	}
}


void executeNoDependencies(GraphNode_t no_dependencies)
{
	struct GraphNode *temp = no_dependencies;
	while(temp != NULL)
	{
		pid_t pid = fork();
		if (pid == 0)
		{
			execute_command(temp->command, true);
			exit(1);
		}
		else
		{
			temp->pid = pid;
		}
		temp = temp->next;
	}
}

void executeDependencies(GraphNode_t dependencies)
{
	struct GraphNode *temp = dependencies;
	while (temp != NULL)
	{
		if (temp->next == NULL)
			break;
		temp = temp->next;
	}

	while(temp != NULL)
	{
		int i = 0;
		otter:
		while(temp->before[i] != NULL)
		{
			if (temp->before[i]->pid == -1)
				goto otter;
			i++;
		}
		int status;
		i = 0;
		while(temp->before[i] != NULL)
		{
			waitpid(temp->before[i]->pid, &status, 0);
			i++;
		}
		pid_t pid = fork();
		if(pid == 0)
		{
			execute_command(temp->command, true);
			exit(0);
		}
		else
		{
			temp->pid = pid;
		}
		temp = temp->prev;
	}
}
