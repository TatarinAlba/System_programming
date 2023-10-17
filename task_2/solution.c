#include "parser.h"
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

static bool
execute_command_line(const struct command_line *line)
{
	/* REPLACE THIS CODE WITH ACTUAL COMMAND EXECUTION */
	// int outputFile = 1;
	// PROBLEM WITH SUCH INPUT ">"
	// PROBLEM WITH INCREMENTING FILE DESCRIPTOR CHEECK
	int outputFIle = 1;
	int stdout_addr = dup(STDOUT_FILENO);
	int stdin_addr = dup(STDIN_FILENO);
	int my_pipes[2];
	assert(line != NULL);
	printf("================================\n");
	printf("Command line:\n");
	printf("Is background: %d\n", (int)line->is_background);
	printf("Output: ");
	if (line->out_type == OUTPUT_TYPE_STDOUT) {
		printf("stdout\n");
	} else if (line->out_type == OUTPUT_TYPE_FILE_NEW) {
		printf("new file - \"%s\"\n", line->out_file);
		// outputFile = open(line->out_file, O_CREAT | O_WRONLY | O_TRUNC, 0666);
	} else if (line->out_type == OUTPUT_TYPE_FILE_APPEND) {
		printf("append file - \"%s\"\n", line->out_file);
		// outputFile = open(line->out_file, O_APPEND | O_CREAT | O_WRONLY);
	} else {
		assert(false);
	}
	const struct expr *e = line->head;
	pipe(my_pipes);
	dup2(my_pipes[0], STDIN_FILENO);
	dup2(my_pipes[1], STDOUT_FILENO);
 	while (e != NULL) {
		if (e->type == EXPR_TYPE_COMMAND) {
			if (strcmp(e->cmd.exe, "cd") == 0) {
				if (chdir(e->cmd.args[0]) != 0) {
					perror("Error durring changing directory");
				}
			}
			else if (strcmp(e->cmd.exe, "exit") == 0 && e->cmd.arg_count == 0) {
				return false;
			}
			else {

				pid_t child_pid;
				child_pid = fork();
				// Child process, here we will execute 
				if (child_pid == 0) {
					close(my_pipes[0]);
					// Question about arrays in execvp and comand
				// as an argument
					char* argc[e->cmd.arg_count + 2];
					for (uint32_t i = 1; i < e->cmd.arg_count + 1; i++) {
						argc[i] = e->cmd.args[i - 1];
					}
					argc[0] = e->cmd.exe;
					argc[e->cmd.arg_count + 1] = NULL;
					execvp(e->cmd.exe, argc);	
					// not best practice, maybe there is another beautiul way
					close(my_pipes[1]);
					close(stdout_addr);
					close(stdin_addr);
					return false;
				} else { 
					// TODO: THINK ABOUT STATUS AND COULD WE USE WAITPID
					// TODO: BACKGROUND PROCESSES
					wait(NULL);
				}
			}
			printf("\tCommand: %s", e->cmd.exe);
			for (uint32_t i = 0; i < e->cmd.arg_count; ++i)
				printf(" %s", e->cmd.args[i]);
			printf("\n");
		} else if (e->type == EXPR_TYPE_PIPE) {
			printf("\tPIPE");
		} else if (e->type == EXPR_TYPE_AND) {
			printf("\tAND\n");
		} else if (e->type == EXPR_TYPE_OR) {
			printf("\tOR\n");
		} else {
			assert(false);
		}
		e = e->next;
	}
	close(my_pipes[1]);
	char buffer[1024];
	ssize_t bytes_read;
	dup2(stdout_addr, STDOUT_FILENO);
	dup2(stdin_addr, STDIN_FILENO);
	while ((bytes_read = read(my_pipes[0], buffer, sizeof(buffer))) > 0) {
		write(STDOUT_FILENO, buffer, bytes_read); // Write to the standard output (stdout)
	}
	close(my_pipes[0]);
	return true;
}

int
main(void)
{
	const size_t buf_size = 1024;
	char buf[buf_size];
	int rc;
	bool executed = true;
	struct parser *p = parser_new();
	while ((rc = read(STDIN_FILENO, buf, buf_size)) > 0) {
		parser_feed(p, buf, rc);
		struct command_line *line = NULL;
		while (true) {
			enum parser_error err = parser_pop_next(p, &line);
			if (err == PARSER_ERR_NONE && line == NULL)
				break;
			if (err != PARSER_ERR_NONE) {
				printf("Error: %d\n", (int)err);
				continue;
			}
			executed = execute_command_line(line);
			command_line_delete(line);
			if (!executed) {
				break;
			}
		}
		if (!executed) {
			break;
		}
	}
	parser_delete(p);
	return 0;
}
