#include "../src/lab.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <termios.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <errno.h>

char *get_prompt(const char *env){

  char *envVariable = getenv(env);

  if(envVariable == NULL){
    setenv(env, "shell>", 1);
    envVariable = getenv(env);
  }

  char *prompt = (char *)malloc(strlen(envVariable) + 1);
  strcpy(prompt, envVariable);
  return prompt;
}

int change_dir(char **dir) {
  char *nextDir;
  if (*dir == NULL || dir == NULL) {
    nextDir = getenv("HOME");
    if (nextDir == NULL){
      fprintf(stderr, "No HOME environment variable set.\n");
      return -1;
    }
  } else {
      nextDir = *dir;
  }

  if(chdir(nextDir) != 0){
    perror("chdir failed");
    return -1;
  }
  return 0;

}

char **cmd_parse(char const *line) {
  
  char *token;
  char *lineCopy = strdup(line);
  int argsCount = 0;
  char **args = NULL;
  long argMax = sysconf(_SC_ARG_MAX);

  token = strtok(lineCopy, " \t\n");
  while (token != NULL && argsCount < argMax){
    args = realloc(args, sizeof(char*) * (argsCount +1));
    args[argsCount] = strdup(token);
    argsCount++;
    token = strtok(NULL, " \t\n");
    
  }
  args = realloc(args, sizeof(char*) * (argsCount +1));
  args[argsCount] = NULL;
  free(lineCopy);
  return args;
}

void cmd_free(char **line) {
  if (line == NULL) return;

  for(int i = 0; line[i] != NULL; i++){
    free(line[i]);
  }
  free(line);
}

char *trim_white(char *line) {
  if(line == NULL) return NULL;

  while(isspace((unsigned char)*line)) line++;

  if(*line == 0) {
      return line;
  }

  char *lineEnd = line + strlen(line) -1;
  while(lineEnd > line && isspace((unsigned char) *lineEnd)){
    lineEnd--;
  }

  lineEnd[1] = '\0';

  return line;
}

bool do_builtin(struct shell *sh, char **argv) {
    if (sh == NULL || argv == NULL || argv[0] == NULL) {
        fprintf(stderr, "Error: Invalid arguments to do_builtin\n");
        return false;
    }

    if (strcmp(argv[0], "exit") == 0) {
        printf("Exiting shell...\n");
        sh_destroy(sh);
        exit(0);
    } else if (strcmp(argv[0], "cd") == 0) {
        if (change_dir(&argv[1]) == 0) {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("Current directory: %s\n", cwd);
            } else {
                perror("getcwd() error");
            }
        } else {
            fprintf(stderr, "cd: failed to change directory\n");
        }
        return true;
    } else if (strcmp(argv[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("getcwd() error");
        }
        return true;
    } else if (strcmp(argv[0], "history") == 0) {
        HIST_ENTRY **hist_list = history_list();
        if (hist_list) {
            for (int i = 0; hist_list[i]; i++) {
                printf("%d: %s\n", i + 1, hist_list[i]->line);
            }
        } else {
            printf("No command history available.\n");
        }
        return true;
    } else if (strcmp(argv[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  exit - Exit the shell\n");
        printf("  cd [dir] - Change directory\n");
        printf("  pwd - Print current working directory\n");
        printf("  history - Display command history\n");
        printf("  help - Display this help message\n");
        return true;
    }

    return false;
}

void sh_init(struct shell *sh) {
    if (sh == NULL) {
        fprintf(stderr, "Error: Shell struct is NULL\n");
        exit(1);
    }

    // Initialize shell members
    sh->shell_terminal = STDIN_FILENO;
    sh->shell_is_interactive = isatty(sh->shell_terminal);

    if (sh->shell_is_interactive) {
        // Loop until we are in the foreground
        while (tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp())) {
            kill(-sh->shell_pgid, SIGTTIN);
        }

        // Ignore interactive and job-control signals
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGCHLD, SIG_IGN);

        // Put ourselves in our own process group
        sh->shell_pgid = getpid();
        if (setpgid(sh->shell_pgid, sh->shell_pgid) < 0) {
            perror("Couldn't put the shell in its own process group");
            exit(1);
        }

        // Grab control of the terminal
        tcsetpgrp(sh->shell_terminal, sh->shell_pgid);

        // Save default terminal attributes for shell
        tcgetattr(sh->shell_terminal, &sh->shell_tmodes);
    }

    // Set up the prompt
    sh->prompt = get_prompt("MY_PROMPT");
}

void sh_destroy(struct shell *sh) {
    if (sh == NULL) {
        return;  // Nothing to do if shell is NULL
    }

    // Free the prompt if it was allocated
    if (sh->prompt != NULL) {
        free(sh->prompt);
        sh->prompt = NULL;
    }

    // If the shell is interactive, restore the terminal settings
    if (sh->shell_is_interactive) {
        // Restore default signal handlers
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        // Restore the terminal attributes
        tcsetattr(sh->shell_terminal, TCSADRAIN, &sh->shell_tmodes);

        // Give control of the terminal back to the default process group
        tcsetpgrp(sh->shell_terminal, tcgetpgrp(sh->shell_terminal));
    }

    // Reset all shell structure members
    sh->shell_terminal = -1;
    sh->shell_is_interactive = 0;
    sh->shell_pgid = -1;
}

int externalCommand(struct shell *sh, char **args) {
    pid_t pid;
    int status;

    pid = fork();

    if (pid < 0) { // Fork failed
        perror("fork");
        return -1;
    } else if (pid == 0) { // Child process
        // Set up the child process
        pid_t child = getpid();
        setpgid(child, child);

        // Reset signal handlers to default
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        // Execute the command
        execvp(args[0], args);
        
        // If execvp returns, it must have failed
        fprintf(stderr, "exec failed\n");
        exit(EXIT_FAILURE);
    } else { // Parent process
        // Wait for the child process to complete
        if (waitpid(pid, &status, 0) == -1) {
            if (errno == ECHILD) {
                // Child has already terminated
                return 0;  // Assume success if we can't get the actual status
            }
            perror("waitpid");
            return -1;
        }

        // Give control back to the shell
        tcsetpgrp(sh->shell_terminal, sh->shell_pgid);

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "Child process terminated by signal %d\n", WTERMSIG(status));

            return -1;
        }
    }
    return 0; // This should not be reached
}