#include<stdio.h>

static char input[2048];

int main(int argc, char const *argv[]) {
  /* code */
  puts("Lispy Version 0.0.0.0.1");
  puts("Please press Ctrl + C to Exit\n");

  while (1) {
    /* code */
    fputs("Lispy> ", stdout);
    fgets(input, 2048, stdin);

    printf("No, you are a %s\n", input);
  }
  return 0;
}
