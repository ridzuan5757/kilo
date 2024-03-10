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

# Improvement in window size implementation

`ioctl()` is not guaranteed to be able to request the window size on all
systems, so we are going to provide a fallback method of getting the window
size.

The strategy is to position the cursor at the bottom-right of the screen,
then use escape sequences that let us query the position of the cursor. That
tells us how many rows and columns there must be on the screen.

```c
int getWindowSize(int *rows, int *cols){
    struct winsize ws;

    if(1 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        if(write(STDOUT_FILENO. "\x1b[999C\x1b[998B", 12) != 12){
            return -1;
        }

        editorReadKey();
        return -1;
    }else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}
```

The implementation of moving the cursor to the bottom-right corner is not
exactly straightforward. We are sending two escape sequences one after the
other. The `C` command **cursor forward** and the `B` command **cursor down**
moves the cursor down. The argument says how much to move it to the right or
down by. We use a very large value, `999`, which should ensure that the cursor
reaches the right and bottom edges of the screen.

The `C` and `B` commands are specifically documented to stop the cursor from
going past the edge of the screen. The reason we do not use the `<esc>[999;999H`
command is that the documentation does not specify what happens if we try to
move the cursor off-screen.

Notice that we are sticking a `1 ||` at the front of the `if` statement
temporarily, so that we can test this fallback branch that we are developing.

Because we are always returning `-1` (meaning an error occurred) from 
`getWindowSize()` at this point, we make a call to `editorReadKey()`, so we can
observe the results of our escape sequences before the program calls `die()` and
clears the screen. When we run the program, we should see the cursor is
positioned at the bottom-right corner of the screen, and when we press a key we
will see the error message printed by `die()` after it clears the screen.

Next we need to get the cursor position. The `n` command (device status report)
can be used to query the terminal for the status information. We want to give an
argument of `6` to ask for the cursor position. Then we can read the reply from
the standard input. Let's print out each character from the standard input to
see what the reply looks like.

```c
int getCursorPosition(int *rows, int* cols){
    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4){
        return -1;
    }

    printf("\r\n");
    char c;
    while(read(STDIN_FILENO, &c, 1) == 1){
        if(iscntrl(c)){
            printf("%d\r\n", c);
        }else{
            printf("%d ('%c')\r\n", c, c);
        }
    }

    editorReadKey();
    return -1;
}

```

The reply is an escape sequence! It is an escape character `27`, followed by a
`[` character, and then the actual response: `24:80R`, or similar. This escape
sequence is documented as Cursor Position Report.

As before, we have inserted a temporary call to `editorReadKey()` to let us
observe our debug output before the screen gets cleared on exit.

If we are using Bash on Windows, `read()` does not time out, so we will be stuck
in an infinite loop. We will have to kill the process externally, or exit and
reopen the command prompt window.

We are going to parse this response. But first, let's read it into the buffer.
We will keep reading characters until we get to `R` character.

```c
int getCursorPosition(int *rows, int *cols){
    char buf[32];
    unsigned int i = 0;

    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4){
        return -1;
    }

    while(i < sizeof(buf) - 1){
        if(read(STDIN_FILENO, &buf[i], 1) != 1){
            break;
        }

        if(buf[i] == 'R'){
            break;
        }

        i++;
    }

    buf[i] = '\0';

    printf("\r\n&buf[1] : '%s'\r\n", &buf[1]);
    editorReadKey();

    return -1;
}
```

When we print out the buffer, we do not want to print the '\x1b' character,
because the terminal would interpret it as an escape sequence and would not
display it. So we skip the first character in `buf` by passing `&buf[1]` to
`printf()`. `printf()` expects strings to end with a `0` byte, so we make sure
to assign `'\0'` to the final byte of `buf`.

If we run the program, we will see we have the response in `buf` in the form of
`<esc>[24;80`. Let's first parse the two numbers out of there using `sscanf()`:

```c
int getCursorPosition(int *rows, int *cols){
    char buf[32];
    unsigned int i = 0;

    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4){
        return -1;
    }

    while(i < sizeof(buf) - 1){
        if(read(STDIN_FILENO, &buf[i], 1) != 1){
            break;
        }

        if(buf[i] == 'R'){
            break;
        }

        i++;
    }
    buf[i] = '\0';

    if(buf[0] != '\x1b' || buf[1] != '['){
        return -1;
    }

    if(sscanf(&buf[2], "%d;%d", rows, cols) != 2){
        return -1;
    }

    return 0;
}

```

`sscanf()` from `<stdio.h>`. First we make sure it responded with an escape
sequence. Then we pass a pointer to the third character of `buf` to `sscanf()`,
skipping the `\x1b` and `[` characters. So we are passing a string of the form
`24;80` to `sscanf()`. We are also passing it the string `%d:%d` which tells it
to parse two integers separated by `;`, and put the values into `rows` and 
`cols` variables.

Our fallback method for getting the window size is now complete. We should see
that `editorDrawRows()` prints the correct number of tildes for the height of
our terminal.

Now that we know how that works, let's remove `1 ||` we put in the `if`
condition temporarily.

```c
int getWindowSize(int *rows, int *cols){
    struct winsize ws;

    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        if(write(STDOUT_FILENO, "\x1b[999C\x1b[998B", 12) != 12){
            return -1;
        }

        return getCursorPosition(rows, cols);
    }else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}
```

# Last line

The last line of the screen does not seem to have a tilde. That is because
of a small bug in our code. When we print the final tilde, we then print
`"\r\n"` like on any other line, but this causes the terminal to scroll in
order to make room for a new, blank line. Let's make the last line an exception
when we print `"\r\n"`'s.

```c
void editorDrawRows(){
    int y;

    for(y = 0; i < E.screenrows; y++){
        write(STDOUT_FILENO, "~", 1);

        if(y < E.screenrows - 1){
            write(STDOUT_FILENO, "\r\n", 2);
        }
    }
}
```

# Append buffer

It is generally not a good idea to make a whole bunch of small `write()`'s 
everytime we refresh the screen. It would be better to do one big `write()`, to
make sure the whole screen updates at once. Otherwise there could be small
unpredictable pauses between `write()`'s, which could cause flicker effect.

We want to replace all of our `write()` calls with code that appends the string
to a buffer, and then `write()` this buffer at our end. Unfortunately, C does
not have dynamic strings, so we will create our own dynamic string type that
implement buffer appending.

```c
struct abuf{
    char *b;
    int len;
}
```

An append buffer consists of a pointer to our buffer in memory, an a lenth. We
define an `ABUF_INIT` constant which represents an empty buffer. This acts as a
constructor for our `abuf` type. We will also implement `abAppend()` operation
as well as `abFree()` destructor.

```c
#include <string.h>

void abAppend(struct abuf *ab, const char *s, int len){
    char *new = realloc(ab->b, ab->len + len);

    if(new == NULL){
        return;
    }

    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab){
    free(ab->b);
}
```

`realloc()` and `free()` come from `<stdlib.h>` while `memcpy` comes from
`<string.h>`.

To append a string `s` to an `abuf`, the first thing we do is make sure we
allocate enough memory to hold the new stirng. We ask `realloc()` to give us
block of memory that is the size of the current string plus size of the string
that we are appending. `realloc()` will either extend the size of the block of
memory we already have allocated, or it will take care of `free()`-ing the
current block of memory and allocating a new block of memory somewhere that is
big enough for our string.

Then we use `memcpy()` to copy the string `s` after the end of the current data
in the buffer, and we update the pointer and length of the `abuf` to the new
values.

`abFree()` is a destructor that deallocates the dynamic memory used by an
`abuf`. Now `abuf` type is ready to be used.

```c
void editorDrawRows(struct abuf *ab){
    int y;
    for(y=0; y<E.screenrows; y++){
        abAppend(ab, "~", 1);

        if(y < E,screenrows - 1){
        abAppend(ab, "\r\n", 2);
        }
    }
}

void editorRefreshScreen(){
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[2J", 4);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    abAppend(&ab, "\x1b[H", 3);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}
```

In `editorRefreshScreen()`, we first initialize a new `abuf` called `ab`, by
assigning `ABUF_INIT` to it. We then replace each occurence of
`write(STDOUT_FILENO, ...)` with `abAppend(&ab, ...)`. We also pass `ab` into
`editorDrawRows()`, so it too can use `abAppend()`. Lastly, we `write()` the
buffer's contents out to standard output, and free the memory used by the
`abuf`.

# Hide cursor when repainting

There is another possible source of flickering effect we have to take care. It
is possible that the cursor might be displayed in the middle of the screen
somewhere for a split second while the terminal is drawing to the screen. To
make sure that does not happen, let's hide the cursor before refreshing the
screen, and show it again immediately after the refresh finishes.

```c
void editorRefreshScreen(){
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[2J", 4);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    abAppend(&ab, "\x1b[H", 3);
    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}
```

We use escape sequence to tell the terminal to hide the terminal to hide and
show the cursor. The `h` and `l` commands (set mode, reset mode) are used to
turn on and turn off various terminal features or modes. The VT100 User Guide
used for this reference does not document `?25` which we use above. It appears
the cursor hiding/showing the cursor, but if the do not, then they just ignore
those escape sequences, which is not a big deal in this case.

