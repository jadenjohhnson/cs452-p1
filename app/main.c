#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "../src/lab.h"
#include <readline/readline.h>
#include <readline/history.h>

int main(int argc, char **argv)
{

  //Next steps: start built in commands, get rid of the switch case, so the shell starts without any argument (unless -v)

  //set prompt
  char *prompt = get_prompt("MY_PROMPT");

  int c;
  char *line;
  while((c = getopt (argc, argv, "v" )) != -1)
    switch (c)
    {
      case 'v':
      //This is step 1: print the lab version number
        printf("Lab Version is: %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
        free(prompt);
        return -1;
        break;
      default:
        break;
    }

  using_history();
  while ((line = readline(prompt)) != NULL){
    if (strcmp(line, "exit") == 0) {
        free(line);
        break;
    }
    printf("%s\n", line);
    add_history(line);
    free(line);
  }
  free(prompt);
  return 0;
}
