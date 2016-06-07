#include<stdio.h>
#include<stdlib.h>

#include<editline/readline.h>


int main(int argc, char const *argv[]) {
  /* code */
  puts("Lispy Version 0.0.0.0.1");
  puts("Please press Ctrl + C to Exit\n");

  while (1) {
    /* code */

    /* output our prompt and get the input*/
    char* input = readline("Lispy> ");
    /* Add input to history*/
    add_history(input);

    printf("No, you are a %s\n", input);
    /* free retrived input*/
    free(input);
  }
  return 0;
}
