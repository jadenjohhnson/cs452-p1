#include "../src/lab.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

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
    if (argv == NULL || argv[0] == NULL) {
        return false;
    }

    if (strcmp(argv[0], "exit") == 0) {
        // Exit command
        exit(0);
    } else if (strcmp(argv[0], "cd") == 0) {
        // cd command
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
        // pwd command
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("getcwd() error");
        }
        return true;
    }
    fprint("Not a built in function");
    return false;
}