/*
 * parse.h
 *
 *  Created on: Mar 4, 2014
 *      Author: lister
 */

#ifndef PARSE_H_
#define PARSE_H_

#define MAX_VAR_NUM 10
#define PIPE_MAX_NUM 10
#define FILE_MAX_SIZE 40

#endif /* PARSE_H_ */

struct commandType {
	char *command;
	char *VarList[MAX_VAR_NUM];
	int VarNum;
};

/* parsing information structure */
typedef struct {
	int boolInfile; /* boolean value - infile specified */
	int boolOutfile; /* boolean value - outfile specified */
	int boolBackground; /* run the process in the background? */
	int boolMultipleCmd; /* Bool to check for Multiple Command seperated by ; */
	struct commandType CommArray[PIPE_MAX_NUM];
	int pipeNum;
	char inFile[FILE_MAX_SIZE]; /* file to be piped from */
	char outFile[FILE_MAX_SIZE]; /* file to be piped into */
} parseInfo;

typedef struct {
	pid_t pid; /* job PID */
	int jid; /* job ID [1, 2, ...] */
	int state; /* UNDEF, BG, FG, or ST */
	struct parseInfo  jobinfo;
}job;

/* the function prototypes */
parseInfo *parse(char *);
void free_info(parseInfo *);
void print_info(parseInfo *);
