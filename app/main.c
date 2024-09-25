#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "../src/lab.h"
#include <readline/readline.h>
#include <readline/history.h>

int main(int argc, char **argv)
{
  // Initialize shell
  struct shell theShell;
  sh_init(&theShell);

  int c;
  char *line;
  while((c = getopt (argc, argv, "v" )) != -1)
    switch (c)
    {
      case 'v':
      //This is step 1: print the lab version number
        printf("Lab Version is: %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
        sh_destroy(&theShell);
        return -1;
        break;
      default:
        break;
    }

  using_history();
  while ((line = readline(theShell.prompt)) != NULL){

    if (strlen(line) > 0) {
      add_history(line);  // Add non-empty lines to history
    }

    char *lineTrimmed = trim_white(line);
    char **args = cmd_parse(lineTrimmed);

    if (!do_builtin(&theShell, args)) {
      //Time for an external command!
      int externResult = externalCommand(args);

      if (externResult != 0) {
            fprintf(stderr, "External Command failed, status: %d\n", externResult);
        } else {
        // fflush(stdout);
        // printf("%s\n", stdout);
        // printf("%s\n",readline(NULL));
        // printf("should print here\n");
        // printf("%s", theShell.prompt);
        }
    }

    cmd_free(args);
    free(line);
  }

  sh_destroy(&theShell);
  return 0;
}
