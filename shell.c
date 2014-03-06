/*
 * shell.c
 *
 *  Created on: Mar 4, 2014
 *      Author: Dinesh Kumar . S
 *      Bits ID: 2012HZ13035
 */
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>

#include "parse.h" /* Custom Parser for the commands */

#define MAX_PROCESS 10

#define MAX_LENGTH 1024

#define MAX_BG_JOBS 10

/* Job states */
#define UNDEF 0 /* undefined */
#define FOREGROUND 1    /* running in foreground */
#define BACKGROUND 2    /* running in background */
#define STOPPED 3    /* stopped */

/* Global variables */
int process_type;
parseInfo *info; //info stores all the information returned by parser.
job *jobs[MAX_BG_JOBS]; // If I had time I'd write a linked list data structure
struct commandType *com; //com stores command name and Arg list for one command.
int numJobs = 0;
int nextjid = 1; /* next job ID to allocate */

int process_style = SEQUENTIAL;

/* Enum variables */
enum BUILTIN_COMMANDS {
	NO_SUCH_BUILTIN = 0, EXIT, JOBS, BG, FG, KILL, STYLE
};

enum PROCESS_STYLE {
	SEQUENTIAL = 0, PARALLEL
};

void process_controller(char *cmdLine);

int isBuiltInCommand(char * cmd);

void set_process_style(parseInfo *info);

void executeCommands(parseInfo *info);

static void sigHandler(int sig) {
	printf("Ouch!\n");
}

static void sigCtrlCHandler(int sig) {
	char line[MAX_LENGTH];
	while (1) {
		printf(
				"\nDo you want to exit the application, Use proper Case. ? y or n ");
		if (!fgets(line, MAX_LENGTH, stdin)) {
			break;
		}
		if (strncmp(&line[0], "y", strlen("y")) == 0) {
			exit(0);
		}
		if (strncmp(&line[0], "n", strlen("n")) == 0) {
			printf("...\n");
			break;
		}
		if (line[0] == '\n' || line[0] == '\0' || isspace(line[0])) {
			continue;
		}
	}

}

int main(int argc, char *argv[]) {
	char line[MAX_LENGTH];

	/* Establish handler for SIGINT Ctrl + c*/
	if (signal(SIGINT, sigCtrlCHandler) == SIG_ERR) {
		printf("signal handler is not registered for Ctrl + c\n");
		exit(EXIT_FAILURE);
	}

	/* Establish handler for SIGINT Ctrl + z*/
	if (signal(SIGTSTP, sigHandler) == SIG_ERR) {
		printf("signal handler is not registered for Ctrl + Z\n");
		exit(EXIT_FAILURE);
	}

	initjobs(job *jobs);

	//pid_t pid[MAX_PROCESS];
	//int i, status;
	while (1) {
		printf("dinesh-shell$ ");
		if (!fgets(line, MAX_LENGTH, stdin)) {
			break;
		}
		if (line[0] == '\n' || line[0] == '\0' || isspace(line[0])) {
			continue;
		}
		process_controller(line);
	}
	return 0;
}

/**
 * Will check for the commands and execute the commands
 */
void process_controller(char *cmdLine) {
	//parse the string and give the struct of the same.
	//calls the parser
	info = parse(cmdLine);
	//print_info(info);
	if (info == NULL) {
		//continue;
		return;
	}

	com = &info->CommArray[0];
	if ((com == NULL) || (com->command == NULL)) {
		free_info(info);
		free(cmdLine);
	}
	//com->command tells the command name of com

	int builtinResult = isBuiltInCommand(com->command);

	switch (builtinResult) {
	case EXIT:
		//HANDLE BACKGROUND JOBS!
		printf("Exiting..\n");
		exit(EXIT_SUCCESS);
	case JOBS:
		printf("JOBS..");
		break;
	case BG:
		printf("BG..");
		break;
	case FG:
		printf("fg..");
		break;
	case KILL:
		printf("Kill..");
		break;
	case STYLE:
		set_process_style(info);
		break;
	case NO_SUCH_BUILTIN:
		//handleExec(info, jobs, &numJobs);
		executeCommands(info);
		break;

	default:
		fprintf(stderr, "ahhh weird isBuiltInCommand() return value, dying.\n");
		exit(EXIT_FAILURE);
	}

	if (!(info -> boolBackground)) // Don't free memory if the job's running
		free_info(info); // in the background.
}

int isBuiltInCommand(char * cmd) {

	// Add I/O redirection to builtins

	if (strncmp(cmd, "exit", strlen("exit")) == 0)
		return EXIT;

	else if (strncmp(cmd, "jobs", strlen("jobs")) == 0)
		return JOBS;

	else if (strncmp(cmd, "bg", strlen("bg")) == 0)
		return BG;

	else if (strncmp(cmd, "fg", strlen("fg")) == 0)
		return FG;

	else if (strncmp(cmd, "kill", strlen("kill")) == 0)
		return KILL;

	else if (strncmp(cmd, "style", strlen("style")) == 0)
		return STYLE;

	return NO_SUCH_BUILTIN;
}

/**
 * Sets the process style
 */
void set_process_style(parseInfo *info) {

	int i;
	char *cmd;
	struct commandType *comm;

	// style command
	comm = &(info->CommArray[0]);
	i = comm->VarNum;

	if (i > 2 || i != 2) {
		printf(
				"Wrong Process style arguments. Ex: style sequential or style parallel\n");
		return;
	}

	cmd = comm->VarList[1];
	if (strncmp(cmd, "sequential", strlen("sequential")) == 0) {
		process_style = SEQUENTIAL;
		printf("Process Style set to Sequential -  %d", process_style);
		return;
	}

	if (strncmp(cmd, "parallel", strlen("parallel")) == 0) {
		process_style = PARALLEL;
		printf("Process Style set to Parallel - %d", process_style);
		return;
	}

	if (strncmp(cmd, "get", strlen("get")) == 0) {
		if (process_style == SEQUENTIAL) {
			printf("Process Style set to Sequential -  %d \n", process_style);
		} else if (process_style == PARALLEL) {
			printf("Process Style set to parallel -  %d \n", process_style);
		}
		return;
	}
	// why it came here?
	printf(
			"Wrong Process style arguments. Ex: style sequential or style parallel\n");
}

/**
 * Executes the commands based on the process style
 */
void executeCommands(parseInfo *info) {

	int i, returnStatus;
	struct commandType *comm;
	if (process_style == SEQUENTIAL) {
		// Manually search the paths, testing for existance of file?
		pid_t pid = fork(); // Change to vfork?

		if (pid == 0) // We are running as the child process
		{
			//print_info(info);

			for (i = 0; i <= info->pipeNum; i++) {
				comm = &(info->CommArray[i]);
				if ((NULL == comm) || (NULL == comm->command)) {
					printf("Command %d is NULL, Unable to execute\n", i);
				} else {
					// Manually search the paths, testing for existance of file?
					pid_t child_pid = fork(); // Change to vfork?
					if (child_pid == 0) // We are running as the child process
					{
						int fdInput, fdOutput;

						if (info->boolInfile) {
							if ((fdInput
									= open(info->inFile, O_RDONLY, S_IRUSR))
									< 0) {
								printf("%s", strerror(errno));
								fprintf(stderr, "tshell: %s: %s\n",
										info->inFile, strerror(errno));
								exit(-1);
							} else {
								if (dup2(fdInput, 0) != 0) {
									printf("%s", strerror(errno));
									fprintf(stderr, "tshell: %s: %s\n",
											info->inFile, strerror(errno));
									exit(-1);
								}
								close(fdInput);
							}
						}

						if (info->boolOutfile) {
							if ((fdOutput = open(info->outFile, O_WRONLY
									| O_CREAT | O_TRUNC, S_IWUSR)) < 0) {
								printf("%s --  Error NUmber\n", strerror(errno));
								printf("%d --  fdOutput NUmber\n", fdOutput);
								fprintf(stderr, "tshell: %s: %s\n",
										info->outFile, strerror(errno));
								exit(-1);
							}
							dup2(fdOutput, 1);
							close(fdOutput);
						}
						//printf("Child created %d -", child_pid);
						execvp(comm->command, comm->VarList);
						exit(EXIT_SUCCESS);
					} else {
						if (info->boolBackground) // Run as a background job
						{
							addjob(jobs, child_pid, info, BACKGROUND);
							waitfg(child_pid);
							jd = getjobpid(jobs, pid);
							if (jd != NULL && jd->state != ST) {
								kill(pid, SIGKILL);
								deletejob(jobs, pid);
							}
							/*if (numJobs < MAX_BG_JOBS)
							 //jobs[(numJobs)++] = info;
							 //
							 else   add the jobs in the list
							 fprintf(stderr,
							 "Too many background jobs! Not executing.");*/
						} else {
							wait(&child_pid);
							/*if (waitpid(child_pid, &returnStatus, WNOHANG)) {
							 //Report child exited with return status 'return'
							 //Remove child (linked list style)
							 printf("Child exited %d -", child_pid);
							 }*/
						}
					}
				}
			}
			exit(EXIT_SUCCESS);
		} else {
			wait(&pid);
		}

	}

	if (process_style == PARALLEL) {

	}
}

/*
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid) {
	int status;
	if (waitpid(-1, &status, 0) < 0) //wait for pid to
		unix_error("waitfg: waitpd error"); //no longer be in FG
}

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(job *job) {
    job->pid = 0;
    job->jid = 0;
    job->jobinfo = UNDEF;
    job->state = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(job *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* addjob - Add a job to the job list */
int addjob(job *jobs, pid_t pid, parseInfo *commandInfo, int state)
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
	    return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}
