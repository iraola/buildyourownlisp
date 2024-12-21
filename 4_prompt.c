#include <stdio.h>

/* Declare a buffer for user input size of 2048 */
static char input[2048];

int main(int argc, char** argv) {
 
  /* Print Version and Exit Information */
  puts("Lispy Version 0.0.0.0.1");
  puts("Press Ctrl+C to Exit\n");
  
  while (1) {
    /* Output */
    fputs("lispy> ", stdout);
    
    /* Read a line of user input of maximum size 2048 */
    fgets(input, 2048, stdin);
    
    /* Echo input back to user */
    printf("This is your prompt %s", input);
  }
  
  return 0;
}
