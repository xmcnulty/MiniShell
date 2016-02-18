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
    
    pid_t pid;
    
    switch (pid = fork()) {
        case -1:
            perror("fork");
            break;
        case 0:
            execvp(block->argv[0], block->argv + 1);
            perror(block->argv[0]);
            exit(-1);
        default:
            waitpid(pid, NULL, 0);
            break;
    }
}


//This function should deallocate an ExecBlock including the ExecBlock(s) it might pipe to. 
void freeExecBlock(ExecBlock* block)
{
    ExecBlock* cur = block;
    ExecBlock* next = NULL;
    
    while (cur != NULL) {
        next = cur->pipe;
        
        free(cur);
        
        cur = next;
    }
}

// main method, take user input and pass it to parse
int main(int arvc, char **argv)
{
    while (1) {
        char* input = (char*) calloc(MAX_LEN, sizeof(char));
        
        input = fgets(input, MAX_LEN, stdin);
        
        // case if user inputs exit.
        if (strcmp(input, "exit") == 0) {
            free(input);
            
            return 0;
        } else {
            ExecBlock* execList = parse(input);
            
            executeCommand(execList);
            
            freeExecBlock(execList);
            
            free(input);
        }
    }
    
	return 0;
}
