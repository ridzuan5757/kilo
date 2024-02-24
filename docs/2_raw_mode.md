# Raw mode

We can use `read()` function to read the key presses from the user.

```c
#include <unistd.h>

int main(){
    char c;
    while(read(STDIN_FILENO, &c, 1) == 1);
    return 0l
}
```

`read()` and `STDIN_FILENO` come from `<unistd.h>`. We are using `read()` to
read 1 byte from the standard input to the variable `c`, and to keep doing it
until there are no more bytes to read. `read()` returns the number of bytes that
it read, and will return `0` when it reaches the end of file.

When we run `./kilo`, the terminal gets hooked up to the standard input, and so
the keyboard input gets read into the `c` variable. However, by default the
terminal starts in **canonical mode/cooked mode**. In this mode, keyboard input
is only sent to the `kilo` program when the user presses `Enter/return`. This is
useful for many programs: it lets the user type in a line of text, use
`Backspace` whenever needed to fix errors until we get the input exactly the way
we want it, and finally press `Enter` to send it to the program. However, this
does not work well for programs with mode complex user interfaces, like text
editors. We want to process each key press as it comes in, so we can respond to
it immediately. What we want is **raw mode**, which can be achieved by manipulating terminal
flags.

To exit the program, we can either:
- `Ctrl-D` to tell `read()` that it has reached `EOF`.
- `Ctrl-C` to signal the process to terminate immediately.

# Quit control

To demonstrate how canonical mode works, we will have the program exit when it
reads `q` key press from the user.

```c 
#include<unistd.h>

int main(){
    char c;
    while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
    return 0;
}
```

To quit the program, we have to type a line of text that includes `q` in it, and
then press enter. The program will quickly read the line of text one character
at time until it reads the `q`, at which point the `while` loop will stop and
the program will exit. Any characters after the `q` will be left unread on the
input queue, and we may see that input being fed into the shell after the
program exits.

# Echo control

We can set terminal's attribute by:
1. Using `tcgetattr()` to read the current attributes into a `termios struct`.
2. Modify the struct manually.
3. Pass the modified `termios struct` to `tcsetattr()` to overwrite the new terminal
   attributes.

`struct termios`, `tcgetattr()`, `tcsetattr`, `ECHO`, and `TCAFLUSH` all come
from `<termios.>h`.


```c
#include <termios.h>
#include <unistd.h>

void enableRawMode(){
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCAFLUSH, &raw);
}

int main(){
    enableRawMode();

    char c;
    while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
    return 0;
}

```

The `ECHO` flag causes each key that is being typed to be printed to the
terminal, so we can see what we are typing. While this could be useful in
canonical mode, we would not be needing it as we are trying to render a user
interface in raw mode. The program is still doing the same thing, it just does
not print whatever that is being typed, similar to `sudo` mode.

After the program quits, depending on the type of shell, we may find the
terminal is still not echoing what we type. However, it still listens to what is
being typed. Just press `Ctrl-C` to start a fresh line of input to our shell,
and type in `reset` and press `Enter`. This resets the terminal back to normal
in most cases. Or we can just restart the terminal emulator.

Terminal attributes can be read into a `termios` struct by `tcgetattr()`. After
modifying them, we can then apply them to the terminal using `tcsetattr()`. The
`TCAFLUSH` argument specifies when to apply the change: in this case, it waits
for all pending output to be written to the terminal, and also discards any
input that has not been read.

The `c_lflag` bit field is for **local flags** that can be described as
miscellaneous flags. The other flag fields are:
- `c_iflag` input flags.
- `c_oflag` output flags.
- `c_cflag` control flags.
which we will have to modify in order to enable raw mode.

The `ECHO` is a bit flag, defined as `00000000000000000000000000001000` in
binary. We use bitwise-NOT operator `~` to get
`11111111111111111111111111110111`. We then bitwise-AND this value with the
flags field, which forces the fourth bit in the flags' field to become `0`, and
causes every other bit to retain its current value.

# Disable raw mode at exit

To restore the terminal's original attributes when the program exits, we have
to:
- Save a copy of the `termios` struct in its original state.
- Use `tcsetattr()` to apply it to the terminal when the program exits.

```c
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

void disableRawMode(){
    tcsetattr(STDIN_FILENO, TCAFLUSH, &orig_termios);
}

void enableRawMode(){
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~ECHO;

    tcsetattr(STDIN_FILENO, TCAFLUSH, &raw);

    int main(){ ... }
}
```
`atexit()` comes from `<stdlib.h>`. We use it to register `disableRawMode()`
function to be called automatically when the program exits, whether it exits by
returning from `main()`, or by calling `exit()` function. This way we can ensure
we will leave the terminal attributes the way we found them when the program
exits.

We store the original terminal attributes in a global variable, `orig_termios`.
We assign the `orig_termios` struct to the `raw` to make copy of it before we
start modifying the terminal properties.

It can be observed that the leftovers input is no longer fed into the shell
after the program quits. This is because `TCAFLUSH` option being passed to
`tcsetattr()` when the program exits. As described earlier, it discards any
unread input before applying the changes to the terminal. This however, does not
happen in Cygwin.

