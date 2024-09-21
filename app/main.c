#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "../src/lab.h"
#include <readline/readline.h>
#include <readline/history.h>

int main(int argc, char **argv)
{

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
    // if (strcmp(line, "exit") == 0) {
    //     free(line);
    //     break;
    // }

    char *lineTrimmed = trim_white(line);
    char **args = cmd_parse(lineTrimmed);

    // if (args[0] != NULL) {
    //   if (strcmp(args[0], "cd") == 0) {
    //       change_dir(&args[1]);
    //   } else {
    //       // this location is for when next commands get implemented
    //       printf("Command not implemented: %s\n", args[0]);
    //     }
    // }
    // printf("%s\n", line);

    if (!do_builtin(sh, args)) {
    // Handle external command (fork, exec, etc.)
}
    cmd_free(args);
    add_history(line);
    free(line);
  }
  free(prompt);
  return 0;
}
