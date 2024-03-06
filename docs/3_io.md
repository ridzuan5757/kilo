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

# Clear the screen from exit

Let's clear the screen and reposition the cursor when the program exits. If an
error occurs in the middle of the rendering the screen, we do not want a
bunch of garbage left over on the screen, and we don't want the error to be
printed wherever the cursor happens to be at that point.

```c
void die(const char *c){
    write(STDOUT_FILENO, "x1b[2J", 4);
    write(STDOUT_FILENO, "x1b[H", 3);

    perror(s);
    exit(1);
}

void editorProcessKeypress(){
    char c = editorReadKey();

    switch(c){
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    }
}

```

We have two exit points we want to clear the screen at:

- `die()`
- When the user presses `Ctrl-Q` to quit.

We could use `atexit()` to clear the screen when our program exits, but then
the error message printed by `die()` would be erased right after printing it.

# Tildes

Let's try draw a column of tildes `~` on the left hand side of the screen, like
`vim` does. In the text editor, we will draw tilde at the beginning of any lines
that come after the end of the file being edited.

```c
void editorDrawRows(){
    int y;
    for(y = 0; y < 24; y++){
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}

void editorRefreshScreen(){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    editorDrawRows();

    write(STDOUT_FILENO, "\x1b[H", 3);
}
```

`editorDrawRows()` will handle drawing each row of the buffer of the text being
edited. For now it draws a tilde in each row, which means that row is not part
of the file and cannot contain any text.

We do not know the size of the terminal yet, so we do not know how many rows we
have to draw. For now, we just draw `24` rows.

After we are done drawing, we do another `<esc>[H` escape sequence to reposition
the cursor back up at the top left-corner.

# Global state

The next goal is to get the size of the terminal, so we know how many rows to
draw in `editorDrawRows()`. But first, let's set up a global `struct` that will
contain the editor state, which we will use to store the width and height of the
terminal. For now let's just put our `orig_termios` global into the `struct`.

```c
struct editorConfig{
    struct termios orig_termios;
}
struct editorConfig E;

void disableRawMode(){
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1){
        die("tcsetattr");
    }
}

void enableRawMode(){
    if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1){
        die("tcgetattr");
    }
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= CS8;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1){
        die("tcsetattr");
    }
}
```

Our global variable containing our editor state is named `E`. We must replace
all occurence of `orig_termios` with `E.orig_termios`;

# Window size

On my systems, we should able to gget the size of the terminal by simply calling
`ioctl()` with `TIOCGWINSZ` request. `TIOCGWINSZ` stands for **T**erminal
**I**nput/**O**utput **C**ontro**l** **G**et **WIN**dow **S**i**Z**e.

```c
#include <sys/ioctl.h>

int getWindowSize(int *rows, int *cols){
    struct winsize ws;

    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        return -1;
    }else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}
```

`ioctl()`, `TIOCGWINSZ`, and `struct winsize` come from `<sys/ioctl.h>`. On
success, `iooctl()` will place the number of columns wide and number of rows
high the terminal is into the given `winsize struct`. On failure, `ioctl()`
returns `-1`. We also check to make sure the valuees it gave back were not `0`,
because apparently that is possible erroneous outcome. If `ioctl()` failed in
either way, we have `getWindowSize()` report failure by returning `-1`. If it
succeeded, we pass the values back by setting the `int` reeferences that were
passed to the function. This is a common approach to haaving functions returning
multiple values in C. It also allows us to use the return value to indicate
success or failure.

Afterwards, we will add `screenrows` and `screencols` to our global editor
state, and call `getWindowSize()` to fill in those values.

```c
struct editorConfig{
    int screenrows;
    int screencols;
    struct termios orig_termios;
}
struct editorConfig E;

void initEditor(){
    if(getWindowSize(&E.screenrows, &E.screencols) == -1){
        die("getWindowSize");
    }
}

int main(){
    enableRawMode();
    initEditor();

    while(1){
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}
```

`initEditor()` will be used to initialize all the fields in `E` struct. This way
we can display proper number of `~` on the screen.

```c
void editorDrawRows(){
    int y;
    for(y = 0; i < E.screenrows; y++){
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}
```
