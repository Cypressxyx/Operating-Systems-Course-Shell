/*
 * Shell in C
 * Jorge Bautista
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <errno.h>

void parseCmdWithRedirection(char **);
void parseCmdWithOutput(char **);
void parseCmdWithRandO(char **);
void parseCmdWithPipe(char **);
void execSingleCmd(char **);

/* Extracts user Command from args */
char *extractCmd(char **argv, int size) {
	char cmd[size];
	char *arg = cmd;
	for(int i = 0; i < size - 2; i++) {
		cmd[i] = *argv[i];
		printf("%s \n", argv[i]);
	}
	cmd[size - 1] = '\0';
	return arg;
}

/* Gets size of args up to first null */
int getCmdSize(char **args) {
  char **cmd = args;
	int cmdSize = 0;
	while (*cmd++)
		cmdSize++;
	return cmdSize;
}

/* Format error messages */
void syserror( const char *s) {
	fprintf ( stderr, "%s\n",s);
	fprintf (stderr, " (%s)\n", strerror(errno));
	exit (1);
}

/* parse user input from command line */
int parseLine(char *line, char **argv) { 
	line[strlen(line) - 1 ] = '\0';

	int cmdType = 1;
  /*
	int size = strlen(line) ;
	char tempLine[size + 3];
	bool pipeSeen = false;
	for(int i = 0; i < size; i++) {
		if (line[i] == '|' && (line[i-1] != ' ' || line[i+1] != ' ' )) {
			tempLine[i] = ' ';
			tempLine[i + 1] = '|';
			tempLine[i + 2] = ' ';
			//cmdType = 4;
			pipeSeen = true;
		}
		else {
				if (pipeSeen == true)
					tempLine[i + 2] = line[i];
				else
					tempLine[i] = line[i];
		}
	}
	
	if (pipeSeen == true) {
		tempLine[size + 2] = '\0';
		line = tempLine;
	}*/

	while (*line) {       
		while (*line == ' ' || *line == '\n')
			*line++ = '\0';

		//Figure out what type of command we may have
		if (*line == '<')  
				cmdType = (cmdType == 3) ? 5:2;
		else if (*line == '>' ) 
				cmdType = (cmdType == 2) ? 5:3;
		else if (*line == '|')
				cmdType = 4;

		*argv++ = line;
		while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n') 
 			line++;
	}
  *argv = '\0';
	return cmdType;
}

int main() {
	char line[1024];
	char *argv[20];
	printf("\n");

	while(1) {
		printf("> ");
		fgets(line,sizeof(line), stdin);
		if(line[0] != '\n') {
		int cmdType = parseLine(line, argv);

		if (strcmp(argv[0], "exit") == 0)
			break;
		else if (cmdType == 1)
			execSingleCmd(argv);
		else if (cmdType == 2)
			parseCmdWithRedirection(argv);
		else if (cmdType == 3)
			parseCmdWithOutput(argv);
		else if (cmdType == 4)
			parseCmdWithPipe(argv);
		else if (cmdType == 5)
			parseCmdWithRandO(argv);
		}
	}
	return 0;
}

void parseCmdWithRedirection(char **args){
  //HERE IS THE PLAN
  //We know that the last two values will be redirection and pipe
  //hence we open the file at the last index  or size - 1
  //and store the command in args up to the redicetion or size - 2!

	int size = getCmdSize(args);
  /* Store command up to the last 2, as those are the < and filename */
  char *argv[size - 1];
	for(int i = 0; i < size - 2; i++) {
		argv[i] = args[i];
		//printf("%s \n", argv[i]);
	}
	argv[size - 2] = '\0'; 

	int readFD;
	pid_t pid;
	switch ( pid = fork() ){
		case -1:
			syserror( "Fork Failed");
		case 0:
			if ( (readFD = open(args[size - 1], O_RDONLY)) < 0) {
				char *buf = ( char *) malloc ( strlen( args[0] ) + strlen (args[size - 1]) + 3);
				sprintf(buf, "%s: %s", args[0], args[size-1] );
				syserror(buf);
			}

			if( dup2 (readFD, 0) == -1 || close(readFD) )
				syserror( "Failed to run dup2 or to close the  file-descriptors in child 1." );
			execvp(argv[0], argv);
			syserror( "excevp failed in the child redirection process. Terminating.");
	}
	while ( wait(NULL) != -1);
	return ;
}

void parseCmdWithOutput(char **argv) {
	int size = getCmdSize(argv);
	char *args[size - 1]; //save argument up to >
	for(int i = 0 ; i < size - 2; i++) {
		args[i] = argv[i];
	}	
	args[size - 2] = '\0';

	
	int openFD;
	pid_t pid;
	switch( pid = fork() ){
		case -1:
			syserror("Fork Failed");
		case 0:
			if ( (openFD = open(argv[size - 1], O_WRONLY | O_TRUNC | O_CREAT ,S_IRWXU | S_IRWXG | S_IRWXO)) < 0 ) {
				char *buf = ( char *)malloc(strlen( argv[0] ) + strlen("output.txt") + 3);
				sprintf(buf, "%s: %s", argv[0], "output.txt");
				syserror(buf);
			}
			
			if( dup2(openFD, 1) == -1 || close(openFD)) {
				syserror("Failed to run dup2 or to close the file descriptors");
			}
			//execlp("ls", "ls", NULL);
			execvp(args[0], args);
			syserror("exec failed in the child process");
	}
	while( wait(NULL) != -1);
	return ;
}

// > then <
void parseCmdWithRandO(char **argv) {
	int size = getCmdSize(argv);
	
	//style: [command] < file > file
	char *args[size - 3];
	for (int i = 0; i < size - 4; i++) {
		args[i] = argv[i];
	}
	args[size - 4] = '\0';

	int openFD;
	int readFD;
	pid_t pid;
	switch(pid = fork()) {
		case -1:
			syserror("Fork failed.");
		case 0:
			if ( (readFD=open(argv[size -3], O_RDONLY)) < 0 ) { //-3 is where the read file is
				syserror("File does not exist");
			}
			if ( (openFD = open(argv[size - 1], O_WRONLY | O_TRUNC | O_CREAT ,S_IRWXU | S_IRWXG | S_IRWXO)) < 0 ) {
				syserror("idk man");
			}
			if ( dup2(readFD,0) == -1 || dup2(openFD, 1) == -1 || close(readFD) == -1|| close(openFD) == -1)
				syserror("Failed to run dup2 or close file descriptors");
			execvp(args[0], args);
			syserror("Process failed??");
	}
	while( wait(NULL) != -1);
	return ;
}

void parseCmdWithPipe(char **argv) {
	int size = getCmdSize(argv);

	char val[1];
	int pipePosition = 0;
	for(int i = 0; i < size; i++ ) {
		strcpy(val, argv[i]);
		if ( val[0] == '|') {
			pipePosition = i;
			break;
		}
	}

	//we now know that the first argument is within [0 -> PipePositon - 1]
	//we also know that the second argument is within [PipePositoon + 1 -> size]
	
	char *argOne[pipePosition + 1];
	for(int i = 0; i < pipePosition ; i++) {
		argOne[i] = argv[i];
	}
	argOne[pipePosition] = '\0';

	char *argTwo[size - pipePosition];
	for(int i = 0; i < (size - pipePosition); i ++) {
		argTwo[i] = argv[i + pipePosition + 1];
	}
	argTwo[size - pipePosition - 1] = '\0';

	int numPipes = 0;
	for(int i = 0;i < size; i++) {
		if (*argv[i] == '|')
				numPipes += 1;
	}
	//printf("num pipes seen was: %d \n", numPipes);
	numPipes = numPipes * 2;

	char *grep[] = {"grep", "3", NULL};
	char *grepT[] = {"wc", NULL};
	char **cmd[] = {argOne, argTwo, grep, grepT,NULL};
	//char **cmd[] = {argOne, argTwo, NULL};
	

	int pfd[2];
	pid_t pid;
	int currentFd = 0;
	int currIter = 1;
	while (*cmd) {

		if ( pipe(pfd) == 1)
				syserror("Could not create a pipe");

		switch( pid = fork() ) {
			case -1:
				syserror( "Fork Failed" );
			case 0:
				if ( dup2(currentFd, 0) == -1 || close(pfd[0]) == -1)
					syserror("Failed to run dup2 or close pfd in child");

				if (* (cmd + currIter)) {
					if ( dup2(pfd[1],1) == -1 || close(pfd[1]) == -1)
						syserror("Failed to send ouput to input of next cmd using dup2");
				}

				execvp((*cmd)[0], *cmd);
				syserror("execvp failed");
		}
		wait(NULL);
		currentFd = pfd[0];
		close(pfd[1]);
		//close(pfd[0]);
		*cmd = *(cmd  + currIter);
		currIter += 1;
	}
	/*
	if ( pipe(pfd) == 1)
		syserror("Could Not create a pipe");
	
	switch ( pid = fork() ) {
		case -1:
			syserror( "Fork Failed" );
		case 0:
			if( dup2(pfd[0],0) == -1|| close (pfd[0]) == -1 || close (pfd[1]) == -1)
				syserror( "Failed to run dup2 or close pfd in second child" );
			execvp(argTwo[0], argTwo);
			syserror("execvp unsuccessful in 2nd child");
	}

	switch( pid = fork() ) {
		case -1:
			syserror( "Fork Failed" );
		case 0:
			if( dup2(pfd[1], 1) == -1 || close (pfd[1]) == -1 || close (pfd[0]) == -1) 
				syserror("failed to run dup2 or close pfd in first child");
			execvp(argOne[0], argOne);
			syserror("execvp unsuccessful in 1st child");
	}

	if(close (pfd[1]) == -1 || close (pfd[0]) == -1) 
		syserror("Failed to close dup2 or pfd in parent process.");
	while ( wait(NULL) != -1);*/
	return ;
}

void execSingleCmd(char **argv) {
	pid_t pid;
	switch ( pid = fork() ) {
		case -1:
			syserror( "Fork failed.");
		case 0: 
			execvp(argv[0], argv);
			syserror( "excevp failed in the command process. Terminating.");
	}
	//parent process continues
	while ( wait(NULL) != -1);
	return ;
}
