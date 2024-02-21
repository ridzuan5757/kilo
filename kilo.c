#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

/**
 * Error handling
 *
 * We will implement function that prints error message and exits the program.
 * `perror()` comes from `<stdio.h>` and `exit()` comes from `<stdlib.h>`.
 *
 * Most C library functions that fail will set the global `errno` variable to
 * indicate that the error was. `perror()` looks at the global `errno` variable
 * and prints a descriptive error message for it. It also prints the string
 * given to it before it prints the error message, which is meant to provide
 * context about what part of the code causing such error.
 *
 * After printing out the error message, we exit the program with status `1`
 * indicating a failure is happening.
 *
 * For every functions that has been implemented, we will check for the calls
 * for failure and then call `die()` when they fail.
 *
 * For every function that return `-1` on error:
 *  - `tcsetAttr()`
 *  - `tcgetattr()`
 *  - `read()`
 *  These function will set the global `errno` value to indicate the error.
 *
 *  In Cygwin, when `read()` times out, it returns `-1` with an `errno` of
 *  `EAGAIN`, instead of returning `0` like it supposed to do. In order to make
 *  this program work on Cygwin, we would not treat `EAGAIN` as error when
 *  calling `read()` in `main()`.
 *
 *  An easy way to make `tcgetattr()` fail is to give the program text file or a
 *  pipe as the standard input instead of our terminal. This can be implemented
 *  such as:
 *
 *      ./kilo <kilo.c>
 *
 *      or
 *
 *      echo test | ./kilo
 *
 *  Both should result in the same error for `tcgetattr`, something like
 *  `Inappropiate ioctl for device`.
 */
void die(const char *s) {
  perror(s);
  exit(1);
}

/**
 * Disabling raw mode.
 *
 * To restore the terminal's orignal attributes when the program exits. This is
 * done by saving a copy of `termios` struct in its original state, and use
 * `tcsetattr()` to apply it to the terminal when the program exits.
 */
void disableRawMode(void) {
  /**
   * To implement the error handling here, we check the return value of
   * `tcsetAttr()` call.
   *
   * Old:
   * tcsetAttr(STDIN_FILENO, TCAFLUSH, &orig_termios)
   *
   * New:
   * if(tcsetAttr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1){
   *    die("tcsetAttr")
   * }
   */
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
    die("tcsetattr");
  }

  /**
   * As `TCSAFLUSH` option being passed to `tcsetattr()` when the program exits,
   * the leftover input will be no longer fed into the shell after the program
   * quit.
   *
   * It discards any unread input before applying change to the terminal.
   */
  // tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
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

  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
    die("tcgetattr");
  }

  /**
   * `atexit()` comes from `<stdlib.h>`. We use it to register the
   * `disableRawMode()` function to be called automatically when the program
   * exits, whether it exits by returning from `main()` or by calling the
   * `exit()` function. This way we can ensure we will leave the terminal
   * attriubtes the way we found them when the program exits.
   */
  // tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disableRawMode);

  /**
   * We store the original terminal attributes in a global variable,
   * `orig_termios`. We then assign the `orig_termios` struct to the `raw`
   * struct in order to make a copy of it before we start making our change.
   */
  struct termios raw = orig_termios;

  /**
   * Software flow control
   *
   * By default, `Ctrl-S` and `Ctrl-Q` are used for software flow control.
   * `Ctrl-S` stops data from being transmitted to the terminal until `Ctrl-Q`
   * is pressed. This originates in the days when we might want to pause the
   * transmission of data to let a device such as printer to catch up.
   *
   * `IXON` comes from `<termios.h>`. The `I` stands for input flag and `XON`
   * comes from the names of the two control characters that `Ctrl-S` and
   * `Ctrl-Q` produce: `XOFF` to pause transmission and `XON` to resume
   * transmission.
   *
   * With `ISIG` disabled:
   *  - `Ctrl-S` can be read as 19 byte data.
   *  - `Ctrl-Q` can be read as 17 byte data.
   *
   *
   *
   * Carriage return and New line control
   *
   * If we run the program now and go through the whole alphabet while holding
   * down `Ctrl`, we should see that we have every letter except `M`. `Ctrl-M`
   * is weird: it is being read as 10, when we expect it to be read as 13, since
   * it is the 13th letter of the alphabet, and `Ctrl-J` already produces 10.
   * `Enter` key also produces byte value `10`.
   *
   * It turns out that the terminal is helfully translating any carriage returns
   * (13, '\r') inputted by the user into newlines (10, '\n'). This can be
   * controlled using `ICRNL` flag. `ICRNL` flag comes from `<termios.h>`. The
   * `I` stands for input flag, `CR` stands for carriage return and `NL` stands
   * for new line. This way `Ctrl-M` is read as `13` (carriage return) as well
   * as `Enter` key.
   */
  raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);

  /**
   * Miscellaneous flags
   *
   * `BRKINT`, `INPCK`, `ISTRIP`, and `CS8` all come from `<termios.h>`. This
   * step probably won't have any observable effect, because these flags are
   * either already turned off, or they do not really apply to modern terminal
   * emulators. But at one point or another, switching them off was considered
   * to be part of enabling "raw mode", so we carry on the tradition in making
   * terminal program.
   *  - When `BRKINT` is turned on, a break condition will cause a `SIGINT`
   *  signal to be sent to the program similar to pressing `Ctrl-C`.
   *  - `INPCK` enables parity checking, which does not seem to apply to modern
   *  terminal emulators.
   *  - `ISTRIP` causes the 8th bit of each input byte to be stripped, meaning
   *  it will set it to `0`.
   *  - `CS8` is not a flag, it is a bit mask with multiple bits, which we set
   *  using bitwise OR. It sets the character size (CS) to 9 bits per byte.
   */
  raw.c_cflag |= CS8;

  /**
   * Output processing control
   *
   * Terminal des a similar translation on the output side. It translate each
   * newline `\n` printed into a carriage return followerd by a newline `\r\n`.
   * The terminalr requires both of these characters in order to start a new
   * line of text. The carriage return moves the cursor back to the beginning of
   * the current line, and the newline moves the cursor down a line, scrolling
   * the screen if necessary. (These two distinct operations originated in the
   * days of typewriter and teletypes.
   *
   * We will turn off all output procefssing features by turning off the `OPOST`
   * flag. In practice, the `\n` to `\r\n` translation is liekly the only output
   * processing feature turned on by default.
   *
   * `OPOST` comes from `<termios.h>. `O` means it is an output flag and `POST`
   * stands for post-processing of output. If we run the program now we will see
   * that the newline characters we are printing are only moving the cursor down
   * instead of pushing the cursor all the way to the left side.
   *
   * This can be fixed by adding carriage return to every print statement. This
   * also means that now we always have to write out the full `\r\n` whenever we
   * want to start a new line.
   */
  raw.c_oflag &= ~(OPOST);

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
   *
   *
   *
   * SIGINT signal control
   *
   * By default, `Ctrl-C` sends a `SIGINT` to the current process which cause it
   * to terminate, and `Ctrl-Z` sends a `SIGTSTP` to the current process which
   * causes it to suspend. This can be controlled using `ISIG` flag from
   * termios. This way `Ctrl-C` and `Ctrl-Z` will be read as byte instead.
   *  - `Ctrl-C` can be read as 3 byte.
   *  - `Ctrl-Z` can be read as a 26 byte.
   *  - This also disables `Ctrl-Y` on macOS, which is like `Ctrl-Z` except it
   *  waits for the program to read input before suspending it.
   *
   *
   *
   * Implementation-defined input processing control
   *
   * On some systems, when we type `Ctrl-V`, the terminal waits for us to type
   * another character and then sends that character literally. For example,
   * before we disabled `Ctrl-C`, we might have been able to type `Ctrl-V` and
   * then `Ctrl-C` to input 3 byte. We can turn off this feature using the
   * `IEXTEN` flag. Turning off `IEXTEN` also fixes `Ctrl-O` in macOS, whose
   * terminal driver is otherwise set to discard that control character.
   * `IEXTEN` comes from `<termios.h>`. Now that `Ctrl-V` disabled, it can be
   * printed as 22 byte and `Ctrl-O` as 15 byte.
   *  - With `IEXTEN` eanbled, the terminal driver might perform additional
   *  processing on input characters beyond basic character transmission. It
   *  might interpret certain input sequences as commands rather than passing
   *  them directly to the application. (Like how `:` works on vim.)
   */
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

  /**
   * Timeout for `read()`
   *
   * Currently, `read()` will wait indefinitely for input from the keyboard
   * before it returns. What if we want to do something like animate something
   * on the screen while waiting for user input? We can set timeout, so that
   * `read()` returns if it does not get any input for certain amount of time.
   *
   * `VMIN` and `VTIME` come from `<termios.h>`. They are indexes into the
   * `c_cc` field, which stands for control characters, an array of bytes that
   * control various terminal setting.
   *
   * The `VMIN` value sets the minimum number of bytes of input needed before
   * `read()` can return. We set it to `0` so that `read()` retruns as soon as
   * there is any input to be read.
   *
   * The `VTIME` value sets the maximum  amount of time to wait before `read()`
   * returns. It is in tenths of second, so we set it to 1/10 of a second  (100
   * milliseconds) . If `read()` times out, it will returns `0`, which makes
   * sense bevause its default return value is the number of bytes read.
   *
   * We will also modify the main program from:
   * int main(void){
   *    enableRawMode();
   *    char c;
   *    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
   *      if (iscntrl(c)) {
   *        printf("%d\r\n", c);
   *      } else {
   *        printf("%d ('%c')\r\n", c, c);
   *      }
   *    }
   * }
   *
   * To:
   * int main(void){
   *    enableRawMode();
   *    while(1){
   *      char c = '\0';
   *      read(STDIN_FILENO, &c, 1);
   *
   *      if(iscntrl(c)){
   *        printf("%d\r\n", c);
   *      }else{
   *        print("%d('%c')\r\n", c, c)
   *      }
   *      if(c == 'q'){
   *        break
   *      }
   *    }
   * }
   *
   * When running the program, we can see how often `read()` times out. If we
   * did not supply any input, `read()` returns without setting the `c`
   * variable, which retains its `0` value and so we see some `0`s getting
   * printed out, which is expected since it is the default return value .
   *
   * When we are typing really fast, we can see that `read()` returns right away
   * for each keypress, so it is not like we can only read one keypress every
   * 0.1 second.
   *
   * If this program is run on Windows Bash, we may see that `read()` still
   * blocked for input. It does not seems to care about the `VTIME` value.
   * Fortunately, this won't make too big of an issue, as we will be basically
   * blocking for input anyways.
   */
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
    die("tcsetattr");
  }
}

int main(void) {
  enableRawMode();

  while (1) {
    char c = '\0';

    /**
     * `errno` and `EAGAIN` comes from `<errno.h>`.
     */
    if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) {
      die("read");
    }

    if (iscntrl(c)) {
      printf("%d\r\n", c);
    } else {
      printf("%d ('%c')\r\n", c, c);
    }

    if (c == 'q') {
      break;
    }
  }

  /*/1** */
  /* * `read()` and `STDIN_FILENO` come from `<unistd.h>`. We are asking */
  /* * `read()` to read 1 bute from the standard input into the variable `c`, */
  /* * and to keep doing it until there are no more bytes to be read. `read()`
   */
  /* * returns the number of bytes that it read, and will return `0` when it */
  /* * reaches the end of the file. */
  /* * */
  /* * When we run `./kilo`, the terminal get hooked up to the standard input,
   */
  /* * and so the keyboard input gets read into the `c` variable. However, by */
  /* * default the terminal starts in CANOCNICAL MODE. In this mode, keyboard */
  /* * input is only sent to the program when the user presses `ENTER`. */
  /* * */
  /* * This is useful for many programs: it lets the user type in a line of */
  /* * text, use `BACKSPACE` to fix errors until they get their input exactly */
  /* * the way they want it, and finally press `ENTER` to send it to the */
  /* * program. But it does not work well for programs with more complex user */
  /* * interfaces, such as text editors. We want to process each keypress as it
   */
  /* * comes in, so we can respond to it immediately. */
  /* * */
  /* * What we want is RAW MODE. Unfortunately, there is no simple switch we can
   */
  /* * flip to set the terminal to raw mode. Raw mode is achieved by turning off
   */
  /* * a great many flags in the terminal. */
  /* * */
  /* * To exit the program, press Ctrl-D to tell the `read()` that it has */
  /* * reached the end of file. Or we can always press Ctrl-C to signal the */
  /* * process to terminal immediately. */
  /* *1/ */
  /*char c; */

  /*/1** */
  /* * To demonstrate how canonical mode works, we will have a program to exit
   */
  /* * when it read a `q` keypress from the user. */
  /* * */
  /* *    while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q'); */
  /* * */
  /* * To quit this program, we will have to type a line of text that include a
   */
  /* * `q` in it, and then press `Enter`. The program will quickly read the line
   */
  /* * of text one character at a time until it reads the `q`, at which the */
  /* * `while` loop will stop and the program will exit. Any characters after */
  /* * the `q` will be leaft unread on the input queue, and we may see that */
  /* * input being fed into the sell after the program exits. */
  /* *1/ */
  /*while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') { */

  /*  /1** */
  /*   * keypress */
  /*   * */
  /*   * To get better idea of how input in raw mode works, lets print out each
   */
  /*   * byte that we `read()`. We will print each character's numeric ASCII */
  /*   * value, as well as character it represents if it is a printable
   * character. */
  /*   * */
  /*   * `iscntrl()` comes from `<ctype.h>`, and `printf()` comes from */
  /*   * `<stdio.h>`. */
  /*   *    - `iscntrl()` tests whether a character is a control character. */
  /*   *    Control characters are non-printable characters that we don't want
   * to */
  /*   *    print to the screen. ASCII codes 0 to 31 are all control characters
   * as */
  /*   *    well as code 127. ASCII codes 32 to 126 are all printable. */
  /*   *    - `printf()` can print multiple representations of a byte. `%d`
   * tells */
  /*   *    it to format the byte as a decimal number (its ASCII code), and `%c`
   */
  /*   *    tells it to write out the byte directly as a character. */
  /*   * */
  /*   * This is now a very useful program. It shows us how various keypress */
  /*   * translate into the bytes we read. Most ordinary keys translate directly
   */
  /*   * into the characters they represent. */
  /*   *    - Arrows keys, `Page Up`, `Page Down`, `Home`, and `End` all input 3
   */
  /*   *    or 4 bytes to terminal: `27`, `[`, and then one or two characters.
   */
  /*   *    This is known as escape secuence. All escape sequences start with a
   */
  /*   *    `27` byte. */
  /*   *    - Pressing `Escape` sends a single 27 byte as input. */
  /*   *    - `Backspace` is byte 127. */
  /*   *    - `Delete` is a 4 byte escape sequence. */
  /*   *    - `Enter` is byte 10, which is a newline character, also known as */
  /*   *    `\n`. */
  /*   *    - `Ctrl-A` is `1`, `Ctrl-B` is `2`. This would means that `Ctrl` key
   */
  /*   *    combinations that do work map the letter A-Z to the codes 1-26. */
  /*   * */
  /*   * Pressing `Ctrl-S` will cause the program to freeze. What is actually */
  /*   * happened is that we have asked the program to stop sending us output.
   */
  /*   * Pressing `Ctrl-Q` tell the program to resume sending the output. */
  /*   * */
  /*   * Pressing `Ctrl-Z`, or `Ctrl-Y` on some machine will cause the program
   * to */
  /*   * be suspended to the background. `fg` command will bring it back to the
   */
  /*   * foreground, but it may quit immediately after we do that, as a result
   * of */
  /*   * `read()` returning `-1` to indicate that an error occured. This is */
  /*   * happening on macOS while Linux seems able to resume the `read()` call
   */
  /*   * properly. */
  /*   *1/ */
  /*  if (iscntrl(c)) { */
  /*    printf("%d\r\n", c); */
  /*  } else { */
  /*    printf("%d ('%c')\r\n", c, c); */
  /*  } */
  /*} */

  /**
   * Running `echo $?` after the program finishes running will return the
   * value of this return statement. This command display the exit status of
   * the previous command. This can be useful to finds any errors in the code
   * at each compilation.
   */
  return 0;
}
