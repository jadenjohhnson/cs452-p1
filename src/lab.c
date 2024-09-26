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
#include <stdint.h>

int backProc = 0;  // Indicate background process

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

  // Check if the last character is '&'
  size_t len = strlen(lineCopy);
  if (len > 0 && lineCopy[len - 1] == '&') {
      backProc = 1;
    if (lineCopy[len - 2] != ' '){
        //fix the line to have a space before '&'
        memmove(lineCopy + len - 1, lineCopy + len - 2, 1); // Shift
        lineCopy[len - 1] = ' '; // Insert a space
        char *newLineCopy = realloc(lineCopy, strlen(lineCopy) + 10);
        if (newLineCopy) {
            lineCopy = newLineCopy;  // Update lineCopy only if realloc is successful
            lineCopy[len] = '&';
            lineCopy[len + 1] = '\0';
        }
    }
  }

  token = strtok(lineCopy, " \t\n");
  while (token != NULL && argsCount < argMax){
    args = realloc(args, sizeof(char*) * (argsCount +1));
    args[argsCount] = strdup(token);
    argsCount++;
    token = strtok(NULL, " \t\n");
    
  }

  // Check if the last argument is '&'
  if (argsCount > 0 && strcmp(args[argsCount - 1], "&") == 0) {
    free(args[argsCount - 1]);  // Free the '&' token
    argsCount--;
    backProc = 1;
  } else{
    backProc = 0;
  }

//   args = realloc(args, sizeof(char*) * (argsCount +1));
    char **temp = realloc(args, sizeof(char*) * (argsCount + 1));
    if (temp) {
        args = temp;
        args[argsCount] = NULL;
    }else {
        fprintf(stderr, "Memory allocation failed\n");
        cmd_free(args);
        free(lineCopy);
        return NULL; // Return NULL on failure
    }
  args[argsCount] = NULL;

//   Store backProc as the last element
//   args[argsCount + 1] = (char*)(intptr_t)backProc;

  free(lineCopy);

  return args;
}

void cmd_free(char **line) {
  if (line == NULL) return;

  for(int i = 0; line[i] != NULL; i++){
    // printf("%s\n",line[i]);
    free(line[i]);
  }
  free(line);
}

char *trim_white(char *line) {
  if(line == NULL) return NULL;

  while(isspace((unsigned char)*line)) line++;

// If the line is all whitespace, return an empty string
if (*line == 0) {
    return "";  // Allocate and return an empty string
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
    } else if (strcmp(argv[0], "jobs") == 0) {
         for(int i = 0; i < sh->job_count; i++){
            printf("[%d] %s\n", sh->jobs[i].id, sh->jobs[i].command);
         }
        return true;
    }  else if (strcmp(argv[0], "help") == 0) {
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

    // Initialize job list
    sh->job_count = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        sh->jobs[i].id = 0;
        sh->jobs[i].pid = 0;
        sh->jobs[i].command = NULL;
        sh->jobs[i].is_background = 0;
    }

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

    // Clean up job list
    for (int i = 0; i < sh->job_count; i++) {
        if (sh->jobs[i].command != NULL) {
            free(sh->jobs[i].command);
            sh->jobs[i].command = NULL;
        }
        // Optionally, you might want to kill any remaining background processes
        if (sh->jobs[i].is_background && sh->jobs[i].pid > 0) {
            kill(sh->jobs[i].pid, SIGTERM);
        }
    }
    sh->job_count = 0;

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
        if(args[0] != NULL){
            execvp(args[0], args);
        }

        // If execvp returns, it must have failed
        fprintf(stderr, "exec failed\n");
        exit(EXIT_FAILURE);
    } else { // Parent process
        if (backProc && args[0] != NULL) {
            add_job(sh, pid, args[0], backProc);
        }

        if (!backProc) {
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
    }
    return 0; // This should not be reached
}

void add_job(struct shell *sh, pid_t pid, char *command, int is_background) {
    if (sh->job_count < MAX_JOBS) {
        sh->jobs[sh->job_count].id = sh->job_count + 1;
        sh->jobs[sh->job_count].pid = pid;
        sh->jobs[sh->job_count].command = strdup(command);
        sh->jobs[sh->job_count].is_background = is_background;
        if (is_background) {
            printf("[%d] %d %s\n", sh->jobs[sh->job_count].id, pid, command);
        }
        sh->job_count++;
    }
}

void check_background_jobs(struct shell *sh) {
    for (int i = 0; i < sh->job_count; i++) {
        if (sh->jobs[i].is_background) {
            int status;
            pid_t result = waitpid(sh->jobs[i].pid, &status, WNOHANG);
            if (result == -1) {
                // Handle error, maybe remove the job if the process doesn't exist
                printf("[%d] Done %s\n", sh->jobs[i].id, sh->jobs[i].command);
                remove_job(sh, sh->jobs[i].id);
            } else if (result == sh->jobs[i].pid) {
                printf("[%d] Done %s\n", sh->jobs[i].id, sh->jobs[i].command);
                remove_job(sh, sh->jobs[i].id);
                i--; // Recheck this index as jobs have shifted
            }
            // If result is 0, the process is still running
        }
        // printf("One job checked.\n");
    }
}

void remove_job(struct shell *sh, int job_id) {
    int found = -1;
    for (int i = 0; i < sh->job_count; i++) {
        if (sh->jobs[i].id == job_id) {
            found = i;
            break;
        }
    }
    
    if (found != -1) {
        free(sh->jobs[found].command);
        for (int j = found; j < sh->job_count - 1; j++) {
            sh->jobs[j] = sh->jobs[j + 1];
            sh->jobs[j].id--; // Adjust the ID of shifted jobs
        }
        sh->job_count--;
    }
}