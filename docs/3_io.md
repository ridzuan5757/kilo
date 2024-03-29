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

# Clearing lines one at a time.

Instead of clearing entire screen before each refresh, it seems more optimal to
clear each line as we redraw them. Let's remove the `<esc>[2J` (clear entire
screen) escape sequence, and instead put a `<esc>[K` sequence at the end of
each line we draw.

```c
void editorDrawRows(struct abuf *ab){
    int y;

    for(y = 0; y < E.screenrows; y++){
        abAppend(ab, "~", 1);


        abAppend(ab, "\x1b[K", 3);
        if(y < E.screenrows - 1){
            abAppend(ab, "\r\n", 2);
        }
    }
}

void editorRefreshScreen(){
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    abAppend(&ab, "\x1b[H", 3);
    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}
```

The `K` (erase in line) command used after printing tilde is used to erase part
of the current line. Its argument is analogous to the `J` command's argument:
- `2` erases the whole line.
- `1` erases the part of the line to the left of the cursor.
- `0` erases the part of the line to the right of the cursor.

`0` is the default argument, and that is what we want, so wee leave out the
argument and just use `<esc>K`.

# Welcome message.

Let's display the name of the editor and a version number a third way down the
screen.

```c
void editorDrawRows(struct abuf *ab){
    int y;
    for(y = 0; y < E.screenrows; y++){
        if(y == E.screenrows / 3){
            char welcome[80];
            int wecomelen = snprintf(
                welcome, 
                sizeof(welcome),
                "Kilo editor -- version %s",
                KILO_VERSION
            );
            
            if(wecomelen > E.screencols){
                wecomelen = E.screencols;
            }

            abAppend(ab, welcome, welcomelen);
        }else{
            abAppend(ab, "~", 1);
        }

        abAppend(ab, "\x1b[K", 3);
        
        if(y < E.screenrows - 1){
            abAppend(ab, "\r\n", 2);
        }
    }
}
```

`snprintf()` comes from `<stdio.h>`. We use the welcome buffer and `snprintf()`
to interpolate our `KILO_VERSION` string into the welcome message. We also
truncate the length of the screen in case the terminal is too tiny to fit the
welcome message.

To center a string, we obtain the padding value by dividing the screen width by
`2`, then we subtract half of the string's length from that. In other words:
`(E.screencols - welcomelen) / 2`. That tells us how far from the edge of the
screen we should start printing the string. So we fill that space with space
characters, except for the first character, which should be tilde.

```c
void editorDrawRows(struct abuf *ab){
   
    int y;
    for(y = 0; y < E.screenrows; y++){
        
        char welcome[80];
        int welcomelen = snprintf(
            welcome,
            sizeof(welcome),
            "Kilo editor -- version %s",
            KILO_VERSION
        );
        
        if(welcomelen > E.screencols){
            welcomelen = E.screencols;
        }

        int padding = (E.screencols - welcomelen) / 2;
        
        if(padding){
            abAppend(ab, "~", 1);
            padding--;
        }

        while(padding--){
            abAppend(ab, " ", 1);
        }

        abAppend(ab, welcome, welcomelen);
    }else{
        abAppend(ab, "~", 1);
    }

    // VT100 K - clear line from cursor right 
    abAppend(ab, "\x1b[K", 3);

    if(y < E.screenrows - 1){
        abAppend(ab, "\r\n", 2);
    }
}
```

# Cursor movement

We want the user to be able to move the cursor around. The first step is to keep
track of the cursor `xy`-position in the global editor state.

```c
struct editorConfig{
    int cx;
    int cy;
    int screenrows;
    int screencols;
    struct termios orig_termios;
};

struct editorConfig E;

void initEditor(){
    E.cx = 0;
    E.cy = 0;

    if(getWindowSize(&E.screenrows, &E.screencols) != -1){
        die("getWindowSize");
    }
}
```

`E.cx` is the horizontal coordinate of the cursor (column) and `E.cy` is the
vertical coordinate (row). We initialize both of them to `0`, as we want the
cursor to start at the top-left of the screen. Since C language uses indexes
that start from `0`, we will use 0-indexed values wherever possible.

`editorRefreshScreen()` will be used to move the cursor to the position stored
in `E.cx` and `E.cy`.

```c
void editorRefreshScreen(){
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}
```

`strlen()` comes from `<string.h>`. We changed the old `H` command into an `H`
command with arguments, specifying the exact position we want the cursor to move
to.

We add `1` to `E.cy` and `E.cx` to convert from 0-indexed values to the
1-indexed values that the terminal uses. At this point, we could try
initializing `E.cx` to `10` or something, or insert `E.cx++` into the main loop,
to confirm the code works as intended sor far. Next, we will allow the user to 
move the cursor using `w`, `a`, `s`, `d` keys.

```c
void editorMoveCursor(char key){
    switch(key){
        case 'a';
            E.cx--;
            break;
        case 'd';
            E.cx++;
            break;
        case 'w';
            E.cy--;
            break;
        case 's':
            E.cy++;
            break;
    }
}

void editorProcessKeypress(){
    char c = editorReadKey();

    switch(c){
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case 'w':
        case 'a':
        case 's':
        case 'd':
            editorMoveCursor(c);
            break;
    }
}
```

# Arrow keys movement

Now that we have a way of mapping keypresses to move the cursor, let's replace
the `WASD` keys with the arrow keys. We saw that pressing an arrow key sends
multiple bytes as input to our program. These bytes are in form of an escape
sequence that start with `\x1b`, `[`, followed by an `A`, `B`, `C`, or `D`
depending on which of the four arrow keys was pressed. We will read escape
sequences of this form as a single keypress.

```c
char editorReadKey(){
    int nread;
    char c;

    while((nread = read(STDIN_FILENO, &c, 1)) != 1){
        if(nread == -1 errno != EAGAIN){
            die("read");
        }
    }

    if(c == '\x1b'){
        char seq[3];

        if(read(STDIN_FILENO, &seq[0], 1) != 1){
            return '\x1b';
        }

        if(read(STDIN_FILENO, &seq[1], 1) != 1){
            return '\x1b';
        }

        if(seq[0] == '['){
            switch(seq[1]){
                case 'A': return 'w';
                case 'B': return 's';
                case 'C': return 'd';
                case 'D': return 'a';
            }
        }

        return '\x1b';
    }else{
        return c;
    }
}
```

If we read an escape character, we immediately read two or more bytes into the
`seq` buffer. If either of these reads time out after 0.1 seconds, then we
assume the user just pressed the `<esc>` key and return that. Otherwise we look
to see if the escape sequence is an arrow key escape sequence. If it is, we just
return the corresponding `WASD` character, for now. If it is not an escape
sequence that we are expecting, we just return the escape character.

We make the `seq` buffer to be 3 bytes long because we might be handling longer
escape sequence in the future. We basically aliased the arrow keys to the `WASD`
keys. This gets the arrow keys working immediately, but leaves `WASD` keys still
mapped to `editorMoveCursor()` function. What we want is for `editorReadKey()`
to return special values for each arrow key that let us identify that a
particular arrow key was pressed.

This can be implemented by first replacing each instance of the `WASD`
characters with the constants `ARROW_UP`, `ARROW_LEFT`, `ARROW_DOWN`,
`ARROW_RIGHT`.

```c
enum editorKey{
    ARROW_LEFT = 'a',
    ARROW_RIGHT = 'd',
    ARROW_UP = 'w',
    ARROW_DOWN = 's'
}

char editorReadKey(){
    int nread;
    char c;

    while((nread = read(STDIN_FILENO, &c, 1)) != 1){
        if(nread == -1 && errno != EAGAIN){
            die("read");
        }
    }

    if(c == '\x1b'){
        char seq[3];

        if(read(STDIN_FILENO, &seq[0], 1) != 1){
            return '\x1b';
        }

        if(read(STDIN_FILENO, &seq[1], 1) != 1){
            return '\x1b'
        }

        if(seq[0] == '['){
            switch(seq[1]){
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
            }
        }
    }else{
        return c;
    }
}


void editorMoveCursor(char key){
    switch(key){
        case ARROW_LEFT:
            E.cx--;
            break;
        case ARROW_RIGHT:
            E.cx++;
            break;
        case ARROW_UP:
            E.cy--;
            break;
        case ARROW_DOWN:
            E.cy++;
            break;
    }
}

void editorProcessKeypress(){
    char c = editorReadKey();

    switch(c){
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
    }
}
```

Now we just have to choose a representation for arrow keys that does not
conflicts with `WASD`, in the `editorKey` enum. We will give them a large
integer value that is out of range of a char, so that they do not conflict with
any other keypresses. We will also have to change all variables that store
keypresses to be of type `int` instead of `char`.

```c
enum editorKey{
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN
}

int editorReadKey();
void editorMoveCursor(int);

void editorProcessKeypress(){
    int c = editorReadKey();
    ///
}
```
By setting the first constant in the enum to be `1000`, the rest of the
constants get incrementing values of `1001`, `1002`, `1003`, and so on.

This concludes the arrow key handling code. At this point, if we try pressing
`<esc>` key, the `[` key, and `Shift+C` in sequence really fast, we may see the
keypresses being interpreted as the arrow key being pressed. However, this is
only possible if the sequence is entered really fast, unless the `VTIME` value
in `enableRawMode()` is adjusted. 
- Pressing `Ctrl-[` is the same as pressing the `<esc>` key.
- Pressing `Ctrl-M` is the same as pressing `Enter/Return`.
- `Ctrl`-key clears the 6th and 7th bits of the character we type in combination
  with it.

# Prevent cursor from moving off screen.

Currently, we can cause the `E.cx` and `E.cy` to either reach negative values or
exceed the window size. We can prevent this by performing boundary check in
`editorMoveCursor()`

```c
void editorMoveCursor(int key){
    switch(key){
        case ARROW_LEFT:
            if(E.cx != 0){
                E.cx--;
            }
            break;

        case ARROW_RIGHT:
            if(E.cx != E.screencols - 1){
                E.cx++;
            }
            break;

        case ARROW_UP:
            if(E.cy != 0){
                E.cy--;
            }
            break;

        case ARROW_DOWN:
            if(E.cy != E.screenrows - 1){
                E.cy++;
            }
            break;
    }
}
```

# `Page Up` and `Page Down` keys.

We also need to detect a few more special keypresses that use escape sequences,
like the arrow keys did. We will start with the `Page Up` and `Page Down` keys.
`Page Up` is sent as `<esc>[5~` and `Page Down` is sent as `<esc>[6~`.

```c
enum editorKey{
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    PAGE_UP,
    PAGE_DOWN
}

int editorReadKey(){
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1)) != 1){
        if(nread == -1 && errno != EAGAIN){
            die("read");
        }
    }

    if(c == '\x1b'){
        char seq[3];

        if(read(STDIN_FILENO, &seq[0], 1) != 1){
            return '\x1b';
        }

        if(read(STDIN_FILENO, &seq[1], 1) != 1){
            return '\x1b';
        }

        if(seq[0] == '['){
            if(seq[1] >= '0' && seq[1] <= '9'){
                
                if(read(STDIN_FILENO, &seq[2], 1) != 1){
                    return '\x1b';
                }

                if(seq[2] == '~'){
                    switch(seq[1]){
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                    }
                }
            }else{
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
            }
        }
        
    }
    return '\x1b';
    }else{
        return c;
    }
}
```

If the byte after `[` is a digit, we will try to read the next byte expecting it
to be `~`. Then we test the digit byte to see if it is `5` or `6`. We will use
`Page Up` and `Page Down` to move the cursor to top and bottom of the screen.

```c
void editorProcessKeypress(){
    int c = editorReadKey();

    switch(c){
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                int times = E.screenrows;
                while(time--){
                    editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
                }
            }
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
    }
}
```

We create code block with that pair of braces so that we are allowerd to declare
the `times` variable since we cannot declare variables directly inside a 
`switch` statement. We simulate the user pressing `Arrow Up` and `Arrow Down` 
keys enough times to move to the top or bottom of the screen. Implementation of
`Page Up` and `Page Down` this way will become lot easier as we will also
implement scrolling function. If we are on a machine with `Fn` key, we may be
able to press `Fn + Arrow Up` and `Fn + Arrow Down` to simulate pressing the
`Page Up` and `Page Down` keys.

# `Home` and `End` keys.

Like previous keys, these keys also send escape sequences. Unlike the previous
keys, there are many different escape sequence that could be sent by these keys,
depending on the operating system, or the terminal emulator.

The `Home` key could be sent as `<esc>[1~`, `<esc>[7~`, `<esc>[H`, or
`<esc>[OH`. Similarly, the `End` key could be sent as `<esc>[4~`, `<esc>[8~`,
`<esc>[F`, or `<esc>[OF`.

```c
enum editorKey{
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
}

int editorReadKey(){
    
    int nread;
    char c;
    while((nread = read()STDIN_FILENO, &c, 1) != 1){
        if(nread == -1 && errno != EAGAIN){
            die("read");
        }
    }

    if(c == '\x1b'){
        
        char seq[3];

        if(read(STDIN_FILENO, &seq[0], 1) != 1){
            return '\x1b';
        }

        if(read(STDIN_FILENO, &seq[1], 1) != 1){
            return '\x1b';
        }

        if(seq[0] == '['){
            if(seq[1] >= '0' && seq[1] <= '9'){
                
                if(read(STDIN_FILENO. &seq[2], 1) != 1){
                    return '\x1b';
                }

                if(seq[2] == '~'){
                    switch(seq[1]){
                        case '1': return HOME_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            }else{
                switch(seq[1]){
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        }else if(seq[0] == 'O'){
            switch(seq[1]){
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }

        return '\x1b';
    }else{
        return c;
    }
}
```

We will make `Home` and `End` to move the cursor to the left and right edges of
the screen.

```c
void editorProcessKeypress(){
    int c = editorReadKey();

    switch(c):
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case HOME_KEY:
            E.cx = 0;
            break;

        case END_KEY:
            E.cx = E.screencols - 1;
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                int times = E.screenrows;
                while(times--){
                    editorMoveCursor(c == PAGE_UP ? ARROW_UP: ARROW_DOWN);
                }
            }
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
}
```

If we are on laptop with `Fn` key, we can press `Fn + Left Arrow` and `Fn +
Right Arrow` to simulate pressing `Home` and `End` keys.

# `Delete` key

`Delete` key simply sends the escape sequence `<esc>[3~` which is now pretty
trivial to implement.

```c
enum editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
}

int editorReadKey(){
    //
    //
    //
    if(seq[2] == '~'){
        switch(seq[1]){
            case '1': return HOME_KEY;
            case '3': return DEL_KEY;
            case '4': return END_KEY;
            case '5': return PAGE_UP;
            case '6': return PAGE_DOWN;
            case '7': return HOME_KEY;
            case '8': return END_KEY;
        }
    }
}
```

`Delete` keypress can also be achieved by pressing `Fn + Backspace`
simultaneously.
