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
it immediately.
