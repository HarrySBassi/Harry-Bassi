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
typedef struct command
{
  //char** cmd;
  char** cmd_and_args;
  int numArgs;


  char* inputRedirectFileName;
  char* outputRedirectFileName;

  bool inputRedirect;


  bool outputRedirect;
  bool outputRedirectAppend;

  bool isPipe;


  // char** cmdToExcicute;
} command;


typedef struct command_list_struct
{
  command** cmd_list;
  int num_cmd;
  char* input_string;
  int num_args_in_input_str; // The total number of args in the input string. Used for error handling.
} command_list_struct;


// Takes the input string and the iterator i.
// Is called when a non whitespace whitespace is hit in when parsing the input string.
// Places all char up to the next whitespace in token.
// Uses *i to keep track of current location in input.
char* set_token(char* input, int* i)
{
  int j = 0;
  char* token = (char*)malloc(sizeof(char) * 512);

  while (input[*i] != ' ' && input[*i] != '\n')
  {

    token[j] = input[*i];

    if (input[*i + 1] == '|')
    {
      j++;
      break;
    }
    if (input [*i] == '|')
    {
      j++;
      break;
    }
    if (input[*i] == '>' && input[(*i) + 1] != '>')
    {
      j++;
      break;
    }
    else if (input[*i] != '>' && input[(*i) + 1] == '>')
    {
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
void pass_token_to_cmd(command* _cmd, char* token, int num_tokens)
{

  if (strcmp(token, "<") == 0)
  {
    _cmd->inputRedirect = true;

  }
  else if (_cmd->inputRedirect && _cmd->inputRedirectFileName == NULL)
  {
    _cmd->inputRedirectFileName = token;

  }
  else if (strcmp(token,">") == 0)
  {
    _cmd->outputRedirect = true;

  }
  else if (strcmp(token, ">>") == 0)
  {
    _cmd->outputRedirect = true;
    _cmd->outputRedirectAppend = true;

  }
  else if (_cmd->outputRedirect && _cmd->outputRedirectFileName == NULL)
  {
    _cmd->outputRedirectFileName = token;

  }
  else if (strcmp(token, "|") == 0)
  {
    _cmd->isPipe = true;
  }
  else
  {
    _cmd->cmd_and_args  = (char**)realloc(_cmd->cmd_and_args, sizeof(char*) * num_tokens);
    _cmd->cmd_and_args[num_tokens - 1] = token;
    _cmd->numArgs++;
  }
}

void print_cmd_list(command_list_struct* _cmd_list)
{
  for (int j = 0; j < _cmd_list->num_cmd; ++j)
  {

    command* cmd =_cmd_list->cmd_list[j];

    printf("cmd = %s\n",cmd->cmd_and_args[0]);
    if (cmd->numArgs > 0)
    {
      printf("args = ");
      for (int i = 1; i < cmd->numArgs + 1; i++)
      {
        printf("%s\n",cmd->cmd_and_args[i]);
      }
    }

    if(cmd->outputRedirect == true)
    {
      printf("outputRedirect == true\n");
      if (cmd->outputRedirectAppend == true)
      {
        printf("outputRedirectAppend == true\n");
      }

      printf("outputRedirectFileName: %s\n", cmd->outputRedirectFileName);

    }

    if(cmd->inputRedirect == true)
    {
      printf("inputRedirect == true\n");
      printf("inputRedirectFileName: %s\n", cmd->inputRedirectFileName);

    }
  }

}

char* get_formatted_input_str(char* input_str)
{
  char* in_str = (char*)malloc(sizeof(char) * strlen(input_str));
  int i = 0;

  while(input_str[i] != '\n' && input_str[i] != '\0')
  {
    in_str[i] = input_str[i];
    i++;
  }

  in_str[i + 1] = '\0';

  return in_str;
}
// Returns an initialized instance of a command struct.
command* get_initialized_cmd()
{
  command* cmd = (command*)malloc(sizeof(command));
  cmd->cmd_and_args  = NULL;
  cmd->numArgs = -1; // Because it will be incremented when cmd is added
  cmd->inputRedirectFileName = NULL;
  cmd->outputRedirectFileName  = NULL;

  cmd->inputRedirect = false;

  cmd->outputRedirect = false;
  cmd->outputRedirectAppend = false;

  bool isPipe = NULL;



  return cmd;
}
command_list_struct* get_initialized_cmd_list()
{
  command_list_struct* _command_list = (command_list_struct*)malloc(sizeof(command_list_struct));
  _command_list->cmd_list = NULL;
  _command_list->num_cmd = 0;
  _command_list->input_string = NULL;
  _command_list->num_args_in_input_str = 0;
  return _command_list;
}

typedef struct cmd_list
{

} cmd_list;

command_list_struct* get_user_cmd()
{

  char input[MAX_INPUT_LEN + 1]; // +1 to account for \n
  printf("sshell$ ");
  fgets(input, MAX_INPUT_LEN, stdin);

  // This code id for tester.
  if (!isatty(STDIN_FILENO))
  {
    printf("%s", input);
    fflush(stdout);
  }
  // End tester code

  command_list_struct* _cmd_list = get_initialized_cmd_list();

  //_cmd_list->cmd_list = get_initialized_cmd();

  int i = 0;
  int num_tokens_per_cmd = 0; // keep track of tokens.
  while (input[i] != '\n' && input[i] != '\0')
  {
    if (input[i] != ' ')
    {

      if (_cmd_list->cmd_list == NULL)
      {
        _cmd_list->cmd_list = (command**)malloc(sizeof(command*));
        *(_cmd_list->cmd_list) = get_initialized_cmd();
        _cmd_list->num_cmd++;
      }

      num_tokens_per_cmd++;
      char* token = set_token(input, &i);
      _cmd_list->num_args_in_input_str++; // Increment the number of total commands. Must be less that 16.
      // printf("token: %s\n", token);

      pass_token_to_cmd(_cmd_list->cmd_list[_cmd_list->num_cmd - 1], token, num_tokens_per_cmd);

      if (strcmp(token,"|") == 0 && _cmd_list->cmd_list != NULL)
      {

        // NULL terminatind old command.
        _cmd_list->cmd_list[_cmd_list->num_cmd - 1]->cmd_and_args  =
            (char**)realloc(_cmd_list->cmd_list[_cmd_list->num_cmd - 1]->cmd_and_args,
                            sizeof(char*) * num_tokens_per_cmd + 1);

        _cmd_list->cmd_list[_cmd_list->num_cmd - 1]->cmd_and_args[num_tokens_per_cmd] = NULL;

        //Adding a new command to the command_list
        _cmd_list->num_cmd++;
        _cmd_list->cmd_list = (command**)realloc(_cmd_list->cmd_list,
                                                 sizeof(command*) * _cmd_list->num_cmd);
        _cmd_list->cmd_list[_cmd_list->num_cmd - 1] = get_initialized_cmd();
        num_tokens_per_cmd = 0;
      }


    }
    i++;

  }
  _cmd_list->cmd_list[_cmd_list->num_cmd - 1]->cmd_and_args  =
      (char**)realloc(_cmd_list->cmd_list[_cmd_list->num_cmd - 1]->cmd_and_args,
                      sizeof(char*) * num_tokens_per_cmd + 1);

  _cmd_list->cmd_list[_cmd_list->num_cmd - 1]->cmd_and_args[num_tokens_per_cmd] = NULL;


  //Maybe fix this later---
  _cmd_list->input_string = input;
  // print_cmd_list(_cmd_list);
  return _cmd_list;
}

// In main, ig outputRedirect is true, then this function should be called. This function redirects the output
// By dup2 - int the given file name to stdout.
void redirect_output(command* _cmd)
{
  int f_out;
  if (_cmd->outputRedirectAppend)
  {
    f_out = open(_cmd->outputRedirectFileName, O_CREAT | O_WRONLY | O_APPEND, S_IRWXU | S_IRWXG | S_IRWXO);
  }
  else
  {
    f_out = open(_cmd->outputRedirectFileName, O_CREAT | O_WRONLY,  S_IRWXU | S_IRWXG | S_IRWXO);
  }

  dup2(f_out, STDOUT_FILENO);
  close(f_out);
}

// In main, ig inputRedirect is true, then this function should be called. This function redirects the input
// By dup2- int the given file name to stdin.
void redirect_input(command* _cmd)
{
  int f_in = open(_cmd->inputRedirectFileName, O_CREAT | O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
  dup2(f_in, STDIN_FILENO);
  close(f_in);
}

void pipe2(command_list_struct* _cmd_list)
{
  int* old_fd = (int*)malloc(sizeof(int)*2);
  int* fd = (int*)malloc(sizeof(int)*2);
  for(int i = 0; i < _cmd_list->num_cmd; i++)
  {
    if(i < _cmd_list->num_cmd - 1)
    {
      pipe(fd);
    }
    pid_t pid = fork();

    if(pid == 0)
    {
      if(i > 0)
      {
        dup2(old_fd[0], STDIN_FILENO);
        close(old_fd[0]);
        close(old_fd[1]);
      }
      if(i <  _cmd_list->num_cmd - 1)
      {
        close(fd[0]);
        dup2(fd[1],STDOUT_FILENO);
        close(fd[1]);
      }
      execvp(_cmd_list->cmd_list[i]->cmd_and_args[0], _cmd_list->cmd_list[i]->cmd_and_args);
    }
    else if(pid > 0)
    {
      if(i > 0)
      {
        close(old_fd[0]);
        close(old_fd[1]);
      }
      if(i < _cmd_list->num_cmd - 1)
      {
        old_fd = fd;
      }
    }
  }
  if(_cmd_list->num_cmd > 1)
  {
    close(old_fd[0]);
    close(old_fd[1]);
  }
}
//THE PIPE FUNCTION
void piping_it(int c1, int c2, command_list_struct* _cmd_list, int last_cmd_output)
{
  int pi[2];
  pipe(pi);
  command* _cmd1 = _cmd_list->cmd_list[c1];
  command* _cmd2 = _cmd_list->cmd_list[c2];
  int status;
  pid_t pid = fork();
  if(pid == 0)
  {
    /* Chile*/
    if(c1 != 0)
    {
      // printf("THis is chile last_cmd_output: %d\n", last_cmd_output);
      dup2(last_cmd_output, STDIN_FILENO);
      fprintf(stderr,"This is last_cmd_: %d\n", last_cmd_output);
      close(last_cmd_output);
    }
    fprintf(stderr,"This is chile p[0]: %d\n", pi[0]);
    fprintf(stderr,"This is chile p[1]: %d\n", pi[1]);
    close(pi[0]); /* Don't need read access to pipe */
    dup2(pi[1], STDOUT_FILENO); /* Replace stdout with the pipe */
    close(pi[1]); /* Close now unused file descriptor */
    execvp(_cmd1->cmd_and_args[0], _cmd1->cmd_and_args);
  }
  else if (pid > 0)
  {
    /*Parent*/

    waitpid(0, &status, 0);
    close(pi[1]);
    fprintf(stderr,"This is parent p[0]: %d\n", pi[0]);
    fprintf(stderr,"This is parent p[1]: %d\n", pi[1]);
    //close(pi[1]);

    if(_cmd2->isPipe)
    {
      //printf("This is chile p[0]: %d\n", _cmd2->isPipe);
      piping_it(c2, c2+1, _cmd_list, pi[0]);
      printf("We are fone with cmd# %d\n", c2);
    }
    else
    {
      fprintf(stderr,"We are in the else!!! %s\n", *_cmd2->cmd_and_args);
      close(pi[1]); /* Don't need write to the pipe */
      if(c1 ==0){
        dup2(pi[0], STDIN_FILENO);
      } else {
        fprintf(stderr,"This is last_cmd_parent: %d\n", last_cmd_output);

        dup2(last_cmd_output, STDIN_FILENO); /*Replace STDIN with the read port of the pipe*/
      }
      fprintf(stderr,"WEEE %d\n", pi[0]);
      execvp(_cmd2->cmd_and_args[0], _cmd2->cmd_and_args);
      //return;
    }
  }
}

// checks if there are no input file for <
bool no_input_file(command_list_struct* _cmd_list) {
  bool no_input = false;
  if(strcmp(_cmd_list->cmd_list[0]->cmd_and_args[0],"<") == 0){
    char err_msg[] = "Error: no input file\n";
    fprintf(stderr, "%s", err_msg);
    no_input = true;
  }
  return no_input;
}



/*For missing command error
 * if & of |  is first token
 * if pipe is last token
 * */
// checks if these commands are missing something.
bool check_for_missing_cmd(command_list_struct* _cmd_list) {
  bool was_missing = false;
  if(strcmp(_cmd_list->cmd_list[0]->cmd_and_args[0],"&") == 0 ||
      strcmp(_cmd_list->cmd_list[0]->cmd_and_args[0],"|") == 0 ||
      _cmd_list->cmd_list[_cmd_list->num_cmd - 1]->isPipe){
    char err_msg[] = "Error: missing command\n";
    fprintf(stderr, "%s", err_msg);
    was_missing = true;
  }
  return was_missing;
}
// checks if any of the commands have too many arguments.
bool check_for_too_many_cmd(command_list_struct* _cmd_list) {
  bool were_too_many_args = false;
  for(int i = 0; i < _cmd_list->num_cmd; i++) {
    if(_cmd_list->cmd_list[i]->numArgs > 16) {
      char err_msg[] = "Error: too many process arguments\n";
      fprintf(stderr, "%s", err_msg);
      were_too_many_args = true;
    }

  }
  return were_too_many_args;
}
int main(int argc, char *argv[])
{
  while (true)
  {
    command_list_struct* _cmd_list = get_user_cmd();

    // This code check id there are too many arguments in any of the commands.
    if(check_for_too_many_cmd(_cmd_list)) {
      continue;
    }
    if(check_for_missing_cmd(_cmd_list)) {
      continue;
    }
    if(no_input_file(_cmd_list)) {
      continue;
    }
    command* _cmd = _cmd_list->cmd_list[0];
    pid_t pid;

    int status;

    if (strcmp(_cmd->cmd_and_args[0], "exit") == 0)
    {
      fprintf(stderr, "Bye...\n");
      exit(0);
    }
    if (strcmp(_cmd->cmd_and_args[0], "pwd") == 0)
    {
      char buf[1000];
      getcwd(buf, 1000);
      //printf("%s\n", buf);
    }
    if (strcmp(_cmd->cmd_and_args[0], "cd") == 0)
    {
      int i = chdir(_cmd->cmd_and_args[1]);
      if( i!=0){
        fprintf(stderr ,"%s","Error: no such directory\n");
      }
      fprintf(stderr,
              "+ completed '%s' [%d]\n",
              get_formatted_input_str(_cmd_list->input_string),
              WEXITSTATUS(i));

    }
    else
    {

      pid = fork();
      if (pid == 0)
      {

        // Output redirection
        if (_cmd->outputRedirect)
        {
          redirect_output(_cmd);
        }
        // Input redirection
        if (_cmd->inputRedirect)
        {
          redirect_input(_cmd);
        }

        if (_cmd_list->cmd_list[0]->isPipe)
        {
          piping_it(0, 1, _cmd_list, 0);
          //pipe2(_cmd_list);
        }
        else
        {
          execvp(_cmd->cmd_and_args[0], _cmd->cmd_and_args);
          fprintf(stderr,"%s", "Error: command not found\n");
          exit(1);
        }

      }
      else if (pid > 0)
      {
        waitpid(-1, &status, 0);

        fprintf(stderr,
                "+ completed '%s': [%d]\n",
                get_formatted_input_str(_cmd_list->input_string),
                WEXITSTATUS(status));
      }
      else
      {
        perror("fork");
        exit(1);
      }
    }


    /// Free the mem in commandList struct.
  }
  return EXIT_SUCCESS;
}
