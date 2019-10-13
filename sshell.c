#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef MAX_INPUT_LEN
#define  MAX_INPUT_LEN 512
#endif

// This struct holds a single command along with its arguments and attributes.
typedef struct command {
        //char** cmd;
        char** cmd_and_args;
        int numArgs;


        char* inputRedirectFileName;
        char* outputRedirectFileName;
        char* PipeToNext;


        bool inputRedirect;
        bool PipeIt;


        bool outputRedirect;
        bool outputRedirectAppend;

        char* input_string;
       // char** cmdToExcicute;
}command;

/*
   typedef struct user_input {
        struct cmd* cmd_list = NULL;
        int num_cmd = 0;
   }user_input;
 */

// Takes the input string and the iterator i.
// Is called when a non whitespace whitespace is hit in when parsing the input string.
// Places all char up to the next whitespace in token.
// Uses *i to keep track of current location in input.
char* set_token(char* input, int* i) {
        int j = 0;
        char* token = (char*)malloc(sizeof(char) * 512);

        while (input[*i] != ' ' && input[*i] != '\n') {

                token[j] = input[*i];

                if (input[*i + 1] == '|') {
                	j++;
                	break;
                }

                if (input [*i] == '|') {
                	j++;
                	break;
                }
		if (input[*i] == '>' && input[(*i) + 1] != '>') {
			j++;
			break;
		} else if (input[*i] != '>' && input[(*i) + 1] == '>') {
			j++;
			break;
		}

		(*i)++;
                j++;
        }
        token[j] = '\0';
        return token;
}

// Takes a token and a command. Based on the token and the state of the, modifies a member variable of the command.
void pass_token_to_cmd(command* _cmd, char* token, int num_tokens) {

	if (strcmp(token, "<") == 0) {
		_cmd->inputRedirect = true;

	} else if (_cmd->inputRedirect && _cmd->inputRedirectFileName == NULL) {
		_cmd->inputRedirectFileName = token;

	} else if (strcmp(token,">") == 0) {
		_cmd->outputRedirect = true;

	} else if (strcmp(token, ">>") == 0) {
		_cmd->outputRedirect = true;
		_cmd->outputRedirectAppend = true;

	} else if (_cmd->outputRedirect && _cmd->outputRedirectFileName == NULL) {
		_cmd->outputRedirectFileName = token;

	} else if (strcmp(token, "|") == 0){
      _cmd->PipeIt = true;
    }
	else {
		_cmd->cmd_and_args  = (char**)realloc(_cmd->cmd_and_args, sizeof(char*) * num_tokens);
		_cmd->cmd_and_args[num_tokens - 1] = token;
		_cmd->numArgs++;
	}
}

void print_cmd(command* cmd) {
        printf("cmd = %s\n",cmd->cmd_and_args[0]);
        if (cmd->numArgs > 0) {
                printf("args = ");
                for (int i = 1; i < cmd->numArgs + 1; i++) {
                        printf("%s\n",cmd->cmd_and_args[i]);
                }
        }

        if(cmd->outputRedirect == true) {
        	printf("outputRedirect == true\n");
        	if (cmd->outputRedirectAppend == true) {
			printf("outputRedirectAppend == true\n");
        	}

        	printf("outputRedirectFileName: %s\n", cmd->outputRedirectFileName);

        }

	if(cmd->inputRedirect == true) {
		printf("inputRedirect == true\n");
		printf("inputRedirectFileName: %s\n", cmd->inputRedirectFileName);


	}
	if(cmd->PipeIt == true){
	  printf("PipeIt = true\n");
	  printf("PipeToNext %s\n", cmd->PipeToNext);
	}
}

char* get_formatted_input_str(char* input_str) {
	char* in_str = (char*) malloc(sizeof(char) * strlen(input_str));
	int i = 0;

	while(input_str[i] != '\n' && input_str[i] != '\0') {
		in_str[i] = input_str[i];
		i++;
	}

	in_str[i + 1] = '\0';

	return in_str;
}
// Returns an initialized instance of a command struct.
command* get_initialized_cmd() {
        command* cmd = (command*) malloc (sizeof(command));
        //cmd->cmd  = NULL;
        cmd->cmd_and_args  = NULL;
        cmd->numArgs = -1; // Because it will be incremented when cmd is added
        cmd->inputRedirectFileName = NULL;
        cmd->outputRedirectFileName  = NULL;
        cmd->PipeToNext = NULL;

        cmd->inputRedirect = false;

	cmd->outputRedirect = false;
	cmd->outputRedirectAppend = false;
	cmd->PipeIt = false;

	cmd->input_string = NULL;

        return cmd;
}

command* get_user_cmd(){

        char input[MAX_INPUT_LEN + 1]; // +1 to account for \n
        printf("sshell$ ");
        fgets(input, MAX_INPUT_LEN, stdin);

        // This code id for tester.
	if (!isatty(STDIN_FILENO)) {
		printf("%s", input);
		fflush(stdout);
	}
	// End tester code
        command* _cmd = get_initialized_cmd();

        int i = 0;
        int num_tokens = 0; // keep track of tokens.
        while (input[i] != '\n' && input[i] != '\0') {
                if (input[i] != ' ') {
                        num_tokens++;
                        char* token = set_token(input, &i);

                        // printf("token: %s\n", token);
			pass_token_to_cmd(_cmd, token, num_tokens);


                }
                i++;

        }
        _cmd->cmd_and_args  = (char**)realloc(_cmd->cmd_and_args, sizeof(char*) * num_tokens + 1);
        _cmd->cmd_and_args[num_tokens] = NULL;
        _cmd->input_string = input;
        // print_cmd(_cmd);
        return _cmd;
}

// In main, ig outputRedirect is true, then this function should be called. This function redirects the output
// By dup2 - int the given file name to stdout.
void redirect_output(command* _cmd) {
	int f_out;
	if (_cmd->outputRedirectAppend) {
		f_out = open(_cmd->outputRedirectFileName, O_CREAT | O_WRONLY | O_APPEND, S_IRWXU | S_IRWXG | S_IRWXO);
	} else {
		f_out = open(_cmd->outputRedirectFileName, O_CREAT | O_WRONLY,  S_IRWXU | S_IRWXG | S_IRWXO);
	}

	dup2(f_out, STDOUT_FILENO);
	close(f_out);
}

// In main, ig inputRedirect is true, then this function should be called. This function redirects the input
// By dup2- int the given file name to stdin.
void redirect_input(command* _cmd) {
	int f_in = open(_cmd->inputRedirectFileName, O_CREAT | O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
	dup2(f_in, STDIN_FILENO);
	close(f_in);
}

void confirm_pipe_read(command*_cmd){
  int read_in = open(_cmd->PipeToNext, O_CREAT | O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
  dup2(read_in, STDIN_FILENO);
  close(read_in);
}

void confirm_pipe_write(command*_cmd){
  int read_out = open(_cmd->PipeToNext, O_CREAT | O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
  dup2(read_out, STDOUT_FILENO);
  close(read_out);
}

int main(int argc, char *argv[])
{
	while (true) {
		command* _cmd = get_user_cmd();

		if (strcmp(_cmd->cmd_and_args[0], "exit") == 0) {
			printf("Bye...\n");
			break;
		}

		pid_t pid;
		int mypipe[2];
		if(_cmd->PipeIt == true) {
          /* Create the pipe. */
          if (pipe(mypipe)) {
            fprintf(stderr, "Pipe failed.\n");
            return EXIT_FAILURE;
          }
        }
      int status;

		pid = fork();
		if (pid == 0) {
          if(_cmd->PipeIt == true) {
            close(mypipe[1]);
            confirm_pipe_read(_cmd);
          }
			// Output redirection
			if (_cmd->outputRedirect) {
				redirect_output(_cmd);
			}
			// Input redirection
			if (_cmd->inputRedirect) {
				redirect_input(_cmd);
			}

			execvp(_cmd->cmd_and_args[0], _cmd->cmd_and_args);
			perror("execvp");
			exit(1);
		}
		else if (pid > 0) {
			waitpid(-1, &status, 0);
            if(_cmd->PipeIt == true) {
              close(mypipe[0]);
              confirm_pipe_write(_cmd);
              execvp(_cmd->cmd_and_args[0], _cmd->cmd_and_args);
          }

			fprintf(stderr, "+ completed '%s': [%d]\n", get_formatted_input_str(_cmd->input_string), WEXITSTATUS(status));
		} else {
			perror("fork");
			exit(1);
		}
	}

        return EXIT_SUCCESS;

}