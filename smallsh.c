/**********************************************************************
 *
 * Gabriel Rice
 * CS344 winter Term
 * smallsh.c
 *
 * This program is a small version of a shell environment.
 * Functionality will include: processing commands, handling signals, 
 * stdin/stdout redirection, foreground and background processes.
 * It will support three built in commands: exit, cd and status.
 *
 * Note: I apologize for the "through composed" nature of this program.
 * I fully intended on breaking it up into functions, however, I 
 * unfortunately ran out of time.
 *
 *********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CHARS 2048
#define MAX_ARGS 512


int fgroundOnly = 1; //bool to determine foreground only mode
void catchSIGTSTP(int signo)
{
	if (fgroundOnly == 1)
	{	//catch foreground cmd ctrl-z(SIGTSTP), set bool to false
		char* message = "Entering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, message, strlen(message));
		fflush(stdout);
		fgroundOnly = 0;
	}

	else //if bool is false, set back to true, exit foreground mode
	{
		char* message = "Exiting foreground-only mode\n";
		write(STDOUT_FILENO, message, strlen(message));
		fflush(stdout);
		fgroundOnly = 1;
	}
}



main ()
{

	int i, argSize, numChars = -5;//number of characters received by getline()
	int background = 0; //bool switch 1 - true, 0 - false
	int status = -5; //exit status indicator aka (childExitMethod)
	int fileD; //file descriptor
	int fileResult; //variables for redirection
	char* fileIn = NULL;
	char* fileOut = NULL;
	char* lineEntered = NULL;
	size_t bufferSize = 0;
	char* argLine[MAX_ARGS];
	memset(argLine, '\0', MAX_ARGS); //init argument array
	char* tokens;
	pid_t spawnPid = -5;


	//set SIGINT to be ignored
	struct sigaction SIGINT_action = {0};
	SIGINT_action.sa_handler = SIG_IGN;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;
	sigaction(SIGINT, &SIGINT_action, NULL);


	//set SIGTSTP to be caught for foreground only mode
	struct sigaction SIGTSTP_action = {0};
	SIGTSTP_action.sa_handler = catchSIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);



	while(1)
	{
		while(1)
		{	
			//Prompt
			printf(": ");
			fflush(stdout);
			numChars = getline(&lineEntered, &bufferSize, stdin);

			if (numChars > MAX_CHARS) //make sure input limit not exceeded
			{
				printf("ERROR: exceeded character limit\n");
				exit(1);
			}

			else if (numChars == -1)//if getline() gets interrupted during read call
			{
				printf("getline() interrupt");
				clearerr(stdin);//clear error and re-prompt
			}
			//keep prompting if new line or comment are entered
			else if (lineEntered[0] == '\n' || lineEntered[0] == '#')
				continue;

			else 
				break;
		} 



		// Replace $$ with PID
		/* Duplicate lineEntered, from getline, find the $$ and replace with conversion
		specifier %d to accept pid int.
		Tack this on to lineEntered with sprintf() before tokenizing.
 		Cited source: https://github.com/Davidqww/smallsh/blob/master/smallsh.c */
		char* tempLine = strdup(lineEntered);
		for (i = 0; i < numChars; i++)
		{
			if (tempLine[i] == '$' && tempLine[i+1] == '$')
			{
				tempLine[i] = '%';
				tempLine[i+1] = 'd';
				sprintf(lineEntered, tempLine, getpid());
				break;
			}
		}
		free(tempLine); 
		//printf("%s\n", lineEntered);

		//Tokenize input to vet out required data for commands
		tokens = strtok(lineEntered, " \n"); //delimit by space, get first token
		i = 0; //init index

		//Separate out the indirection operators
		while (tokens != NULL)
		{
			if (strcmp(tokens, "<") == 0)
			{
				tokens = strtok(NULL, " \n");//get argument after space
				fileIn = strdup(tokens);//copy argument as file
				tokens = strtok(NULL, " \n");//move up
			}

			else if (strcmp(tokens, ">") == 0)
			{
				tokens = strtok(NULL, " \n");
				fileOut = strdup(tokens);
				tokens = strtok(NULL, " \n");
			}

			else
			{	//add the rest of the stuff to the array
				argLine[i] = tokens;
				i++;
				tokens = strtok(NULL, " \n");
			}
		}

		argSize = i; //Size of the arg array

		//make sure argument size does not exceed limit
		if (argSize > MAX_ARGS)
		{
			printf("ERROR: too many arguments");
			exit(1);
		}


/*		//Test tokens
		printf("ARGSIZE: %d\n", argSize);
		for (i = 0; i < argSize; i++)
		{
			printf("TOKEN: %s\n", argLine[i]);
		}
*/		



		/***&***/
		//Set bool for background checking
		if (strcmp(argLine[argSize - 1], "&") == 0)
		{
			background = 1; //true
			//now get rid of it out of the array so it won't get passed to execvp()
			argLine[argSize - 1] = NULL;
		}



		/***Exit***/
		if (strcmp(argLine[0], "exit") == 0)
		{
			exit(0);
		}


		/***cd***/ 
		else if (strcmp(argLine[0], "cd") == 0)
		{
			if (argSize == 1) //if they only entered cd
			{
				chdir(getenv("HOME")); //go to HOME dir
				//printf("Dude! it worked!\n");
			} 
			else if (argSize == 2)
			{
				chdir(argLine[1]); //change to dir in next arg
			}
		}


		/***Status***/
		//output: exit status -or- terminating signal of process
		else if (strcmp(argLine[0], "status") == 0)
		{
			//If no foreground started, output exit status 0
			if (WIFEXITED(status))
			{
				printf("exit status: %d\n", WEXITSTATUS(status));
				fflush(stdout);
			}
			else if (WIFSIGNALED(status))//Killer signal
			{
				printf("terminate signal: %d\n", status);
				fflush(stdout);
			}
		}


		//fork() exec() and whatnot
		else
		{

			spawnPid = fork(); 
			if (spawnPid == -1)
			{
				perror("You've been assimilated by the Borg!\n");
				exit(1);
			}
			if (spawnPid == 0) //Its alive!!
			{
				if (background == 0)//not in bg
				{
					//Lets set SIGINT to default mode so we can kill kids
					SIGINT_action.sa_handler = SIG_DFL;
					sigaction(SIGINT, &SIGINT_action, NULL);
				}



				//input redirection
				if (fileIn != NULL)
				{
					fileD = open(fileIn, O_RDONLY);

					//file no good
					if (fileD == -1)
					{ perror("Bad File"); exit(1); }
						
					//set stdin(0) to go to same place fileD points				
					fileResult = dup2(fileD, 0);
					if (fileResult == -1) { perror("dup2"); exit(1); }

					close(fileD); //close file
				}

				//check if in background mode
				else if (background == 1)
				{
					fileD = open("/dev/null", O_RDONLY);
					if (fileD == -1)
					{ perror("Bad File"); exit(1); }
					fileResult = dup2(fileD, 0);
					if (fileResult == -1) { perror("dup2"); exit(1); }

					close(fileD);
				}


				//output redirection
				if (fileOut != NULL)
				{
					fileD = open(fileOut, O_WRONLY | O_CREAT | O_TRUNC, 0644);
					if (fileD == -1)
					{ perror("Bad File"); exit(1); }
					
					//set stdout(1)
					fileResult = dup2(fileD, 1);
					if (fileResult == -1) { perror("dup2"); exit(1); }

					close(fileD);
				}


				//execute commands from first argument
				if (execvp(argLine[0], argLine) != 0);
				{
					fprintf(stderr, "Bad Command: %s\n", argLine[0]);
					fflush(stdout);
					exit(1);
				}
			}


			else //Mother forker!!
			{
				//Foreground processes:
				if (background == 0) //not in bg
				{
					pid_t actualPid = waitpid(spawnPid, &status, 0);
				}

				else if (background == 1 && fgroundOnly == 1)
				{
					pid_t actualPid = waitpid(spawnPid, &status, WNOHANG);
					printf("Background Process ID: %d\n", spawnPid);
					fflush(stdout);
				}
			}
			
		}

		//reset everything 
		free(lineEntered);
		lineEntered = NULL; 

		//reset array for next go'round
		memset(argLine, '\0', MAX_ARGS);
		
		//reset bool and file vars
		background = 0; 
		fileIn = NULL;
		fileOut = NULL;


		//Need to check if children have been killed
		spawnPid = waitpid(-1, &status, WNOHANG);
		while (spawnPid > 0)
		{	//by exit value/status
			if (WIFEXITED(status) != 0)
			{
				printf("Background terminated, pid: %d exit status: %d\n", spawnPid, WEXITSTATUS(status));
				fflush(stdout);
			}
			
			//by signal
			else if (WIFSIGNALED(status) != 0)
			{	
				printf("Background terminated, pid: %d exit signal: %d\n", spawnPid, WTERMSIG(status));
				fflush(stdout);
			}
			spawnPid = waitpid(-1, &status, WNOHANG);

		}

	}
}
