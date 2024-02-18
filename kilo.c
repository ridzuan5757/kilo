#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

/**
 * Disabling raw mode.
 *
 * To restore the terminal's orignal attributes when the program exits. This is
 * done by saving a copy of `termios` struct in its original state, and use
 * `tcsetattr()` to apply it to the terminal when the program exits.
 */
void disableRawMode(void) {

  /**
   * As `TCSAFLUSH` option being passed to `tcsetattr()` when the program exits,
   * the leftover input will be no longer fed into the shell after the program
   * quit.
   *
   * It discards any unread input before applying change to the terminal.
   */
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

/**
 * Turning off echo
 *
 * We can set a terminal's attributes by:
 *  - Using `tcgetattr()` to read the current attributes into a struct.
 *  - Modifying the struct by hand, and passing the modified struct to
 *  `tcsetattr()` to write the new terminal attributes back out.
 *
 *  `struct termios`, `tcgetattr()`, `tcsetattr`, `ECHO`, `TCFLUSH` all come
 *  from `termios.h`.
 *
 *  The `ECHO` feature causes each key typed to be printed in terminal, so we
 *  can see what we are typing. This is useful in canonical mode, but can gets
 *  in a way when we are trying to carefully render a user interface in raw
 *  mode. So we turn it off. This program does the same thing as one in the
 *  previous step, it just does not print what we are typing. This is similar
 * to typing password in terminal when entering `sudo` mode.
 *
 *  After the program quits, depending on the shell, we may find the terminal
 * is still not echoing what we type, however it will still listen to what we
 *  type. Pressing `Ctrl-C` to start fresh line of input, and then type
 * `reset` and press `Enter`. This will resets the terminal in most cases.
 *
 *  Terminal attributes can be read into a `termios` struct via `tcgetattr()`.
 *  After modifying them, we can then apply them to the terminal using
 *  `tcsetattr()`. The `TCSAFLUSH` argument specifies when to apply the
 * change: in this case, it waits for all pending output to be written to the
 * terminal, and also discards any input that has not been read.
 *
 *  The `c_lflag` field is for "local flags" that can be described as "dumping
 *  ground for other state" or as a "miscellaneous flags". The other flag
 * fields a `c_iflag` (input flags), `c_oflag` (output flags) and `c_cflag`
 * (control flags), all of which we will have to modify to enable raw mode.
 *
 *  `ECHO` is a bitflag, defined as `00000000000000000000000000001000` in
 *  binary. We use the bitwise NOT operator on this flag to get
 *  `11111111111111111111111111110111` and then AND-ed with the miscellaneous
 *  flag field, which forces the fourth bit in the flags field become `0`, and
 *  while every other bit to retain its current value.
 */
void enableRawMode(void) {
  /**
   * `atexit()` comes from `<stdlib.h>`. We use it to register the
   * `disableRawMode()` function to be called automatically when the program
   * exits, whether it exits by returning from `main()` or by calling the
   * `exit()` function. This way we can ensure we will leave the terminal
   * attriubtes the way we found them when the program exits.
   */
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disableRawMode);

  /**
   * We store the original terminal attributes in a global variable,
   * `orig_termios`. We then assign the `orig_termios` struct to the `raw`
   * struct in order to make a copy of it before we start making our change.
   */
  struct termios raw = orig_termios;

  /**
   * Canonical mode control
   *
   * There is `ICANON` flag that allows us to turn off canonical model. This
   * means we will finally be reading input byte-byte-byte, instead of
   * line-by-line.
   *  - OLD: raw.c_lflag &= ~ECHO
   *  - NEW: raw.c_flag &= ~(ECHO | ICANON)
   *
   *
   * `ICANON` comes from `<termios.h>`. Input flags (the ones in the `c_iflag`
   * field) generally start with `I` like `ICANON` does. However, `ICANON` is
   * not an input flag, it is a local flag in `c_lflag` field. Now the program
   * will quit as soon as `q` is pressed.
   */
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(void) {
  enableRawMode();

  /**
   * `read()` and `STDIN_FILENO` come from `<unistd.h>`. We are asking
   * `read()` to read 1 bute from the standard input into the variable `c`,
   * and to keep doing it until there are no more bytes to be read. `read()`
   * returns the number of bytes that it read, and will return `0` when it
   * reaches the end of the file.
   *
   * When we run `./kilo`, the terminal get hooked up to the standard input,
   * and so the keyboard input gets read into the `c` variable. However, by
   * default the terminal starts in CANOCNICAL MODE. In this mode, keyboard
   * input is only sent to the program when the user presses `ENTER`.
   *
   * This is useful for many programs: it lets the user type in a line of
   * text, use `BACKSPACE` to fix errors until they get their input exactly
   * the way they want it, and finally press `ENTER` to send it to the
   * program. But it does not work well for programs with more complex user
   * interfaces, such as text editors. We want to process each keypress as it
   * comes in, so we can respond to it immediately.
   *
   * What we want is RAW MODE. Unfortunately, there is no simple switch we can
   * flip to set the terminal to raw mode. Raw mode is achieved by turning off
   * a great many flags in the terminal.
   *
   * To exit the program, press Ctrl-D to tell the `read()` that it has
   * reached the end of file. Or we can always press Ctrl-C to signal the
   * process to terminal immediately.
   */
  char c;

  /**
   * To demonstrate how canonical mode works, we will have a program to exit
   * when it read a `q` keypress from the user.
   *
   *    while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
   *
   * To quit this program, we will have to type a line of text that include a
   * `q` in it, and then press `Enter`. The program will quickly read the line
   * of text one character at a time until it reads the `q`, at which the
   * `while` loop will stop and the program will exit. Any characters after
   * the `q` will be leaft unread on the input queue, and we may see that
   * input being fed into the sell after the program exits.
   */
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q')
    ;

  /**
   * Running `echo $?` after the program finishes running will return the
   * value of this return statement. This command display the exit status of
   * the previous command. This can be useful to finds any errors in the code
   * at each compilation.
   */
  return 0;
}
