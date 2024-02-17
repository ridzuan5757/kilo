#include <unistd.h>

int main() {

  /**
   * `read()` and `STDIN_FILENO` come from `<unistd.h>`. We are asking `read()`
   * to read 1 bute from the standard input into the variable `c`, and to keep
   * doing it until there are no more bytes to be read.
   *
   * `read()` returns the number of bytes that it read, and will return `0` when
   * it reaches the end of the file.
   */
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1)
    ;

  /**
   * Running `echo $?` after the program finishes running will return the value
   * of this return statement. This command display the exit status of the
   * previous command. This can be useful to finds any errors in the code at
   * each compilation.
   */
  return 0;
}
