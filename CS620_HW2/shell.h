
#ifndef SHELL_H
#define SHELL_H
#define MAX_LEN 256
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <readline/readline.h>


typedef struct ExecBlock ExecBlock;
struct ExecBlock 
{
	ExecBlock *pipe; //Pointer to the command to pipe to 
	pid_t threadId; //Pid of the process running the command 
	char **argv; //Arguments for the command
	char *fileIn; //The name of the file to read from
	char *fileOut; //The name of the file to output to
};


ExecBlock* parse(char*);
void executeCommand(ExecBlock*);
void freeExecBlock(ExecBlock*);

int redirectIn(char * fileIn); // redirects input from a file
int redirectOut(char * fileOut); // redirects output to a file

#endif