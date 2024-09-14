#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "../src/lab.h"
#include <readline/readline.h>
#include <readline/history.h>

int main(int argc, char **argv)
{
  // printf("hello world\n");

  //set prompt for when shell is started
  setenv("MY_PROMPT", "(.)(.)", 1);
  char *prompt = getenv("MY_PROMPT");
  
  int c;
  char *line;
  while((c = getopt (argc, argv, "abcvt" )) != -1)
    switch (c)
    {
      case 'a':
        printf("get a here\n");
        break;
      case 'b':
        printf("get b here\n");
        break;
      case 'c':
        printf("get c here\n");
        break;
      case 'v':
      //This is step 1: print the lab version number
        printf("Lab Version Major is: %d\n", lab_VERSION_MAJOR);
        printf("Lab Version minor is: %d\n", lab_VERSION_MINOR);
        return -1;
        break;
      case 't':
        //I will be testing readline here, just tryin to print what I type
        using_history();
        while ((line=readline(prompt))){
          if (strcmp(line, "exit") == 0) {
              free(line);
              break;
          }
          printf("%s\n", line);
          add_history(line);
          free(line);
          }
          break;
      case '?':
        // if (isprint(optopt))
          fprintf(stderr, "Unkown option");
      //     else  fprintf(stderr, "Unknown option", optopt);
        return 1;
      default:
        printf("reached default");
    }
  return 0;
}
