#include<stdio.h>
#include<stdlib.h>

#include<editline/readline.h>

//https://www-s.acm.illinois.edu/webmonkeys/book/c_guide/

int main(int argc, char const *argv[]) {
  /* code */
  puts("Lispy Version 0.0.0.0.1");
  puts("Please press Ctrl + C to Exit\n");

  while (1) {
    /* code */

    /* output our prompt and get the input*/
    char* input = readline("lispy--> ");
    /* Add input to history*/
    add_history(input);

    printf("So, what you just typed is %s\n", input);
    printf("整数 %d\n", 100);
    printf("字符 %c\n", 'x');
    printf("浮点数 %.2f\n", 1.005);
    printf("十六进制 %x\n", 100);
    printf("八进制 %o\n", 100);
    printf("%e\n", 1000.006);
    /* free retrived input*/
    free(input);
  }
  return 0;
}
