#include<stdio.h>

void print_hello_world(int n) {
  /* code */
  for(int count=0; count<n; count++){
    puts("Hello World!");
  }
}
int main(int argc, char const *argv[]) {
  /* code */
  char* hello_world = "Hello World!";
  for (int i=0; i<5; i++){
    puts(hello_world);
  }
  printf("%s\n", "starting while loop");
  int loop_counter = 0;
  while (loop_counter < 5) {
    /* code */
    puts(hello_world);
    loop_counter ++;
  }

  return 0;
}
