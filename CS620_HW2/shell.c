#include "shell.h"

// Parses command line input and creats an ExecBlock list from it.
ExecBlock* parse(char* word)
{
	char *block = strsep(&word, "|");
	if(!block)
		return 0;
	int wordCount = 0, inWord = 0;
	for (char *i = block; *i; i++)
	{
		if (*i == '<' || *i == '>')
			break;
		if (!inWord && *i != ' ' && *i != '\t' && *i != '\n')
			inWord = 1;

		if (inWord && (*i == ' ' || *i == '\t' || *i == '\n'))
		{
			inWord = 0;
			wordCount++;
		}
	}
	if(inWord) wordCount++;
	ExecBlock *out = malloc(sizeof(ExecBlock));
	if (!out) {
		perror("Failed to allocate memory in parse\n");
		exit(0);
	}
	inWord = 0;
	int inInput=0, inOutput=0, wordIndex=0;
	char *wordBase = NULL;
	out->argv = malloc(sizeof(char*) * (wordCount+1));
	out->argv[wordCount] = 0;
	out->fileIn = 0;
	out->fileOut = 0;
	for(char *i = block; *i; i++)
	{
		
		if (!inWord && *i != ' ' && *i != '\t' && *i != '<' && *i != '>' && *i != '\n')
		{
			inWord = 1;
			wordBase = i;
		}
		if (inWord && (*i == ' ' || *i == '\t' || *i == '<' || *i == '>' || *i == '\n'))
		{
			inWord = 0;
			if(inOutput) {
				out->fileOut = wordBase;
				inOutput = 0;
			} else if (inInput) {
				out->fileIn = wordBase;
				inInput = 0;
			} else
				out->argv[wordIndex++] = wordBase;
			if(*i == '<')
				inInput = 1;
			if(*i == '>')
				inOutput = 1;
			*i = '\0';
		}
		if(*i == '<')
			inInput = 1;
		if(*i == '>')
			inOutput = 1;
	}
	if (inWord)
	{
		if(inOutput) {
			out->fileOut = wordBase;
		} else if (inInput) {
			out->fileIn = wordBase;
		} else
			out->argv[wordIndex++] = wordBase;
	}
	out->pipe = parse(word);
	return out;
}


void executeCommand(ExecBlock* block)
{
    if (block == NULL) { // check for null case
        return;
    }
    
    pid_t pid = 0;
    int status = 0;
    int command_count = 0; // number of commands executed
    ExecBlock * _n = block; // current execblock to execute
    
    // count the number of pipes that need to be created
    int num_pipes = 0;
    
    for (ExecBlock * n = block; n; n = n->pipe) num_pipes ++;
    
    // array of pipe file descriptors
    int pipefds[2 * num_pipes];
    
    // create all the pipes necessary for execution
    for (int i=0; i < num_pipes; i++) {
        if (pipe(pipefds + 2*i) < 0) {
            perror("pipe");
            exit(-1);
        }
    }
    
    while (_n) { // execute while there are still commands
        // fork a new process to run the command
        pid = fork();
        
        if (pid == 0) {
            // if there is a command to pipe to
            if (_n->pipe) {
                if (dup2(pipefds[command_count + 1], 1) < 0) {
                    perror("dup2");
                    exit(-1);
                }
            } else if (_n->fileOut) { // redirect output if needed
                if (redirectOut(_n->fileOut) < 0) {
                    perror("redirect out");
                    exit(-1);
                }
            }
            
            if (_n->fileIn) { // redirect input from file if needed
                if (redirectIn(_n->fileIn) < 0) {
                    perror("redirect in");
                    exit(-1);
                }
            }
            
            // if this isn't the first command in the block change
            if (command_count != 0) {
                if (dup2(pipefds[command_count - 2], 0) < 0) {
                    perror("dup2");
                    exit(-1);
                }
            }
            
            // close all the children of this
            for (int i=0; i < 2 * num_pipes; i++) {
                close(pipefds[i]);
            }
            
            // execute the command
            if (execvp(_n->argv[0], _n->argv) < 0) {
                perror("execvp");
                exit(-1);
            }
        } else if (pid < 0) {
            perror("fork");
            exit(-1);
        }
        
        _n = _n->pipe; // progress to the next command block
        command_count += 2; // increment by 2, two fd for each pipe command
    }
    
    // wait for children after closing pipes
    for (int i=0; i < 2*num_pipes; i++) {
        close(pipefds[i]);
    }
    
    for (int i=0; i < num_pipes; i++) {
        wait(&status);
    }
    
    printf("\r"); // start new line in terminal
}

// redirect input from a file
int redirectIn(char * fileIn) {
    FILE* filePtr = fopen(fileIn, "r");
    
    int res = dup2(fileno(filePtr), STDIN_FILENO);
    fclose(filePtr);
    
    return res;
}

// redirect output to another file
int redirectOut(char * fileOut) {
    FILE* filePtr = fopen(fileOut, "w+");
    
    int res = dup2(fileno(filePtr), STDOUT_FILENO);
    fclose(filePtr);
    
    return res;
}

//This function should deallocate an ExecBlock including the ExecBlock(s) it might pipe to. 
void freeExecBlock(ExecBlock* block)
{
    ExecBlock* cur = block;
    ExecBlock* next = NULL;
    
    while (cur != NULL) {
        next = cur->pipe;
        
        free(cur->argv);
        free(cur);
        
        cur = next;
    }
}

// main method, take user input and pass it to parse
int main(int arvc, char **argv)
{
    while (1) {
        printf("MiniShell $ ");
        char* input = (char*) calloc(MAX_LEN, sizeof(char));
        
        input = fgets(input, MAX_LEN, stdin);
        
        // case if user inputs exit.
        if (strcmp(input, "exit\n") == 0) {
            free(input);
            
            return 0;
        } else if (strcmp(input, "\n") != 0) {
            ExecBlock* execList = parse(input);
            
            executeCommand(execList);
            
            freeExecBlock(execList);
            
            free(input);
        }
    }
    
	return 0;
}
