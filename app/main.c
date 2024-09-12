#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  printf("hello world\n");
  
  int c;
  while((c = getopt (argc, argv, "abcv" )) != -1)
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
        printf("This is the version of the project!!\n");
        break;
      case '?':
      //   if (isprint(optopt))
      //     fprintf(stderr, "Unkown option", optopt);
      //     else  fprintf(stderr, "Unknown option", optopt);
        return 1;
      default:
        printf("reached default");
    }
  return 0;
}
