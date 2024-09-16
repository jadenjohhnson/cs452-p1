#include "../src/lab.h"
#include <stdio.h>
#include <string.h>

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