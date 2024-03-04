# Quit control

We saw that the `Ctrl` key combined with the alphanumeric keys seemed to map
bytes 1 to 26. We can use this to detect `Ctrl` key combinations and map them to
different operations in our editor. We will start by mapping `Ctrl-Q` to quit
the operation.

```c
#define CTRL_KEY(k) (k & 0x1f)

int main(){
    while(1){
        char c = '\0';

        if(read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN){
            die("read");
        }

        if(iscntrl(c)){
            printf("%d\r\n", c);;
        }else{
            printf("%d ('%c')\r\n", c, c);
        }

        if(c == CTRL_KEY('q')){
            break;
        }

    return 0;
}
```

The `CTRL_KEY` macro bitwise-ANDs a character with value `00011111`. In C, we
generally specify bit masks using hexadecimal, since C does not have binary
literals, and hexadecimal is more concise and readable. In other words, it sets
the upper 3 bits of the character to `0`. This mirrors what the `Ctrl` key does
in terminal: it strips bits `5` and `6` from whatever key that we press in
combination with `Ctrl`, and sends that.

By convention, bit numbering starts from 0. The ASCII character set seems to be
designed this way on purpose. It is also similarly designed so that we can set
and clear bit 5 to switch between lowercase and uppercase.

# Keyboard input refactor

Let's make a function for low-level key press reading, and another function for
mapping key presses to editor operations. We will also stop printing out
key presses at this point.

```c
char editorReadKey(){
    int nread;
    char c;

    while((nread = read(STDIN_FILENO, &c, 1)) != 1){
        if(nread == -1 && errno != EAGAIN){
            die("read");
        }
    }

    return c;
}

void editorProcessKeypress(){
    char c = editorReadKey();

    switch(c){
        case CTRL_KEY('q'):
            exit(0);
            break;
    }
}

int main(){
    enableRawMode();

    while(1){
        editorProcessKeypress();
    }

    return 0;
}
```

`editorReadKey()`'s job is to wait for one key press, and return it. Later, we
will expand this function to handle escape sequences, which involves reading
multiple bytes that represent single key press, as is the case with arrow keys.

`editorProcessKeypress()` waits for a key press, and then handles it. Later, it
will map various `Ctrl` key combinations and other special keys to a different
editor functions, and insert any alphanumeric and other printable keys'
characters into that is being edited.

Note that `editorReadKey()` is part of `terminal` section because it deals with
the low level terminal input section because it deals with mapping keys to
editor functions at much higher level.

# Screen clear

We are going to render the editor's user interface to the screen after each
key press. Let's start by just clearing the screen.

```c
void editorRefreshScreen(){
    write(STDOUT_FILENO, "\x1b[2J", 4);
}

int main(){
    enableRawMode();

    while(1){
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}
```

`write()` and `STDOUT_FILENO` come from `<unistd.h>`. The `4` in our `write()`
call means we are writing `4` bytes out to the terminal. The first byte is 
`\x1b`, which is the escape character, or `27` decimal. The other three bytes
are `[2J`.

We are writing an **escape sequence** to the terminal. Escape sequences always
start with an escape character `27` followed by a `[` character. Escape
sequences instruct the terminal to do various text formatting tasks, such as
text coloring text, moving the cursor around, and clearing parts of the screen.

We are using the `J` command to clear the screen. Escape sequence commands take
arguments, which come before the command. In this case, the argument is `2`,
which says to clear the entire screen:

- `<esc>[1J` would clear the screen up to where the cursor is.
- `<esc>[0J` would clear the screen from the cursor up to the end of the 
  screen.
- Also, `0` is the default argument for `J`, so just by `<esc[J` by itself
  would also clear the screen from the cursor to the end.

For our text editor, we will be mostly using VT100 escape sequences, which are
supported very widely by modern terminal emulators. See the VT100 User Guide for
complete documentation for each escape sequence.

If we wanted to support the maximum number of terminals out there, we would use
`ncurses` library, which uses the `terminfo` database to figure out the
capabilities of a terminal and what escape sequences to use for that particular
terminal.

# Reposition the cursor

We may notice that the `<esc>[J2` command left the cursor at the bottom of the
screen. Let's reposition it at the top-left so that we are ready to draw
the editor interface from top to bottom.

```c
void editorRefreshScreen(){
    write(STDOUT_FILENO, "x1b[2J", 4);
    write(STDOUT_FILENO, "x1b[H", 3);
}
```
This escape sequence is only `3` byte long, and use `H` (cursor position)
command to position the cursor. The `H` command actually takes 2 arguments:

- The row number 
- The Column number

at which to position the cursor. So if we have an 80 * 24 size terminal and we
want the cursor in the center of the screen, we could use the command
`<esc>[12;40H`. (Multiple arguments are separated by `;` character). The default
arguments for `H` both happen to be `1`, so we can leave both arguments out and
empty for this case as it will position the cursor at the first row and the
first column.
