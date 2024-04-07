# Line viewer

Let's create a data type for storing a row of text in our editor.

```c
typedef struct erow{
    int size;
    char *chars;
} erow;

struct editorConfig{
    int cx;
    int cy;
    int screenrows;
    int screencols;
    int numrows;
    erow row;
    struct termios orig_termios;
};

struct editorConfig E;

void initEditor(){
    E.cx = 0;
    E.cy = 0;
    E.numrows = 0;

    if(getWindowSize(&E.screenrows, &E.screencols) == -1){
        die("getWindowSize");
    }
}
```

`erow` stands for "editor row", and stores a line of text as a pointer to the
dynamically-allocated character data and length. The `typedef` lets us refer to
the type as `erow` instead of `struct erow`.

We add an `erow` value to the editor global state, as well as `numrows`
variable. For now, the editor will only display single line of text, and so
`numrows` can be either `0` or `1`. We initialize it to `0` in `initEditor()`.

Let's fill that `erow` with some text for now. We would not worry about reading
from a file just yet. Instead, we will hardcode a "Hello world" string into it.

```
#include <sys/types.h>

void editorOpen(){
    char *line = "Hello, world!";
    ssize_t linelen = 13;

    E.row.size = linelen;
    E.row.chars = malloc(linelen + 1);
    memcpy(E.row.chars, line, linelen);
    E.row.chars[linelen] = '\0';
    E.numrows = 1
}

int main(){
    enableRawMode();
    initEditor();
    editorOpen();

    while(1){
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}
```

`malloc()` comes from `<stdlib.h>`. `ssize_t` comes from `<sys/types.h>`.
`editorOpen()` will eventually be for opening and reading file from disk. To
load our "Hello, world!" message into the editor's `erow` `struct`, we set the
`size` field to the length of our message, allocate the necessary memory using
`malloc()`, and `memcpy()` the message to the `chars` field which points to the
memory we allocated. Finally, we set the `E.numrows` variable to `1`, to
indicate that the `erow` now contains a line that should be displayed. To
display the text, we placed the row string into the `abuf` buffer when drawing
the rows.

```c
void editorDrawRows(struct abuf *ab){
   int y;
   for(y = 0; y < E.screenrows; y++){
    if(y >= E.numrows){
        if(y == E.screenrows / 3){
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
    }else{
        int len = E.row.size;
        if(len > E.screencols){
            len = E.screencols;
        }
        abAppend(ab, E.rows.chars, len);
    }

    abAppend(ab, "\x1b[K", 3);
    if(y < E.screenrows - 1){
        abAppend(ab, "\r\n", 2);
    }
   }
}
```

We wrap our previous row-drawing code in an `if` statment tha checks whether we
are currently drawing a row that is part of the text buffer, or a row that comes
after the end of the text buffer.

To draw a row that's part of the text buffer, we simply write out the `chars`
field of the `erow`. But first, we take care to truncate the rendered line if it
would go past the end of the screen.

Next, let's allow the user to open an actual file. We will read and display the
first line of the file.

```c
void editorOpen(char *filename){
    FILE *fp = fopen(filename, "r");
    if(!fp){
        die("fopen");
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    linelen = getline(&line, &linecap, fp);

    if(linelen != -1){
        while(linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] ==
        '\r')){
            linelen--;
        }

        E.row.size = linelen;
        E.row.chars = malloc(linelen + 1);
        memcpy(E.rows.chars, line, linelen);
        E.row.chars[linelen] = '\0';
        E.numrows = 1;
    }
    free(line);
    fclose(fp);
}

int main(int argc, char **argv){
    enableRawMode();
    initEditor();

    if(argc >= 2){
        editorOpen(argv[1]);
    }

    while(1){
        editorRefreshScreen();
        editorProcessKeypress();
    }
}
```

`FILE`, `fopen()`, and `getline()` come from `<stdio.h>`. The core of
`editorOpen()` is the same, we just get the `line` and `linelen` values from
`getline()` now instead of hardcoded values.

`editorOpen()` now takes a filename and opens the file for reading using
`fopen()`. We allow the user to choose a file to open by checking if they
passed a filename as a command line argument. If they did, we call 
`editorOpen()` and pass it the filename. If they ran `./kilo` without any
arguments, `editorOpen()` will not be called and they will start with a blank
file.

`getLine()` is useful for reading lines from a file when we do not know how much
memory to allocate for each line. It takes care of memory management for us.
First, we pass it to a null pointer `line` and `linecap` (line capacity) of `0`. 
That makes it allocate new memory for the next line it reads, and:
- set `line` to point to the memory.
- set `linecap` to let us know how much memory it allocated.

It's return value is the length of the line it read, or `-1` if it's at the end
of the file and there are no more lines to read. Later, when we have
`editorOpen()` read multiple lines of a file, we will be able to feed the new
`line` and `linecap` values back into `getline()` over and over, and it will try
and resue the memory that `line` points to as long as the `linecap` is big
enough to fit the next line it reads. For now, we just copy the one line it
reads into `E.rows.chars`, and then `free()` the `line` that `getline()`
allocated.

We also strip off the newline or carriage return at the end of the line before
copying it into our `erow`. We know each `erow` represent one line of text, so
there is no use of storing a newline characters at the end of each one.

If the compiler complains about `getLine()`, we may need to define feature test
macro. Even if it compiles fine on our machine without them, lets just implement
this macro to make the code more portable.

```c
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
...
```

We add them above our includes, because the header files we are including use
the macros to decide what features to expose.

Now let's fix a quick bug. We want the welcome message to only display when the
user starts the program with no arguments, and not when they open a file, as the
welcome message could get in the way of displaying the file.

```c
void editorDrawRows(struct abuf *ab){
    int y;

    for(y = 0; y < E.screenrows; y++){

        if(y > E.numrows){
            
            if (E.numrows == 0 && y == E.screenwos / 3){

                char welcome[80];
                int welcomelen = snprintf(
                    welcome,
                    sizeof(welcome),
                    "Kilo editor -- version %s",
                    KILO_VERSION
                );

                if(welcomelen > E,screencols){
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
            }else{
                int len = E.row.size;

                if(len > E.screencols){
                    len = E.screencols;
                }

                abAppend(ab, E.row.chars, len);
            }

            abAppend(ab, "\x1b[K", 3);
            
            if(y < E.screenrows - 1){
                abAppend(ab, "\r\n", 2);
            }
        }
    }
}
```

Now the welcome message only displays if the text buffer is compeltely empty.

# Multiple Lines Display

To store multiple lines, let's make `E.row` an array of `erow` structs. It will
be a dynamically-allocated array, so we will make it a pointer to `erow`, and
initialize the pointer to `NULL`. This will break bunch of the code that does
not expect `E.row` to be a pointer, so the program will fail to compile at
first.

```c
struct editorConfig{
    int cx;
    int cy;
    int screenrows
    int screenCols;
    int numrows;
    erow *row;
    struct termios orig_termios
};

void initEditor(){
    if(getWindowSize(&E.screenrows. &E.screenCols) == -1){
    die("getWindowSize");
    }
}
```

In `editorOpen()` we will initializes `E.row` to a new function called
`editorAppendRow()`.

```c
void editorAppendRow(char *s, size_t len){
    E.row.size = len;
    E.row.chars = malloc(len +1);
    memcpy(E.row.chars, s, len);
    E.row.chars[len] = '\0';
    E.numrows = 1;
}

void editorOpen(char *filename){
    FILE *fp = fopen(filename, "r");

    if(!fp){
        die("fopen");
    }
    

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    
    linelen = getline(&line, &linecap, fp);
    if(linelen != -1){
        while(linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1])){
            linelen--;
        }

        editorAppendRow(line, linelen);
    }
}
```

We will move the code in `editorOpen()` that initializes `E.row` to a new
function called `editorAppendRow()`.

```c
void editorAppendRow(char *s, size_t len){
    E.raw.size = len;
    E.row.chars = malloc(len + 1);
    memcpy(E.row.chars, s, len);
    E.row.char[len] = '\0';
    E.numrows = 1;
}

void editorOpen(char *filename){
    FILE *fp = fopen(filename, "r");
    if(!fp){
        die("fopen");
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    
    linelen = getline(&line, &linecap, fp);
    if(linelen != -1){
        while(linelen > 0 && 
            (line[linelen - 1] == '\n' || line[linelen - 1] =='\r')){
            linelen--;
        }

        editorAppendRow(line, linelen);
    }
}
```

Notice that we neamed the `line` and `linelen` to variable `s` and `len`, which
are now arguments to `editorAppendRow()`.

We want `editorAppendRow()` to allocate space for a new `erow`, and then copy
the given string to a new `erow` at the end of the `E.row` array.

```c
void editorAppendRow(char *s, size_t len){
    E.row = realloc(E.row, sizeof(erow) * (E.numrows  + 1));


    int at = E.numrows;
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    E.numrows++;
}
```

We have to tell `realloc()` how many bytes we want to allocate, so we multiply
the number of bytes each `erow` takes for `sizeof(erow)`  and multiply that by
the number of rows we want. Then we set `at` to the index of the new row we want
to initialize, and replace each occurence of `E.row` to `E.row[at]`. Lastly, we
update the value of the number of rows, from `E.numrows = 1` to `E.numrows++`.

We will also update `editorDrawRows()` to use `E.row[y]` instead of `E.row`,
when printing out the current line.

```c
void editorDrawRows(struct abuf *ab){
    int y;

    for(y = 0; y < E.screenrows; y++){
        if(y >= E.numrows){
            if(E.numrows == 0 && y == E.screenrows / 3){
                
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
        }else{
            imt len = E.row[y].size;
            
            if(len > E.screencols){
                len = E.screencols;
            }

            abAppend(ab, E.row[y].chars, len);

        }

        abAppend(ab, "\x1b[K", 3);

        of(y < E.screenrows - 1){
            abAppend(ab, "\r\n, 2");
        }
    }
}
```

At this point, the code should compile, but it still only reads a single line
fromt he file. We will add `while` loop to `editorOpen()` to read entire file
into `E.row`.

```c
void editorOpen(char* filename){
    FILE *fp = fopen(filename, "r");
    if(!fp){
        die("fopen");
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while((linelen = getline(&line, &linecap, fp)) != -1){
        while(linelen > 0 && 
            (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')){
            linelen--;
        }

        editorAppendRow(line, linelen);
    }

    free(line);
    fclose(fp);
}
```

The `while` loop works because `getline()` returns `-1` when it reaches end of
the file and there are no more lines to be read.

# Vertical scrolling

Next we want to enable the user to scroll through the whole file, instead of
just being able to see the top few line of the file. Let's add row offset
`rowoff` variable to the global editor state, which will kepp track of what row
of the file the user is currently scrolled to.

```c
struct editorConfig{
    int cx;
    int cy;
    int rowoff;
    int screenrows;
    int screencols;
    int numrows;
    erow *row;
    struct termios orig_termios
};

void initEditor(){
    E.cx = 0;
    E.cy = 0;
    E.rowoff = 0;
    E.numrows = 0;
    E.row = NULL;

    if(getWindowSize(&E.screenrows, &E.screencols) == -1){
        die("getWindowSize");
    }
}
```

We initialize it to `0`, which means we will be scrolled to the top of the file
by default. Now let's have `editorDrawRows()` display the currect range of lines
of the file according to the value of `rowoff`.

```c
void editorDrawRows(struct abuf *ab){
    int y;

    for(y = 0; y < E.screenrowsl y++){

        int filerow = y + E.rowoff;

        if(filerow >= E.numrows){
            if(E.numrows == 0 && y == E.screenrows / 3){
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
                    padding --;
                }

                while(padding--){
                    abAppend(ab, " ", 1);
                }

                abAppend(ab, welcome, welcomelen);
            }else{
                abAppend(ab, "~", 1);
            }
        }else{
            int len = E.row[filerow].size;

            if(len > E.screencols){
                len = E.screencols;
            }

            abAppend(ab, E.row[filerow].chars, len);
        }

        abAppend(ab, "\x1b[K", 3);
        if(y < E.screenrows - 1){
            abAppend(ab, "\r\n", 2);
        }
    }
}
```

To get the row of the file that we want to display at each `y` position, we add
`E.rowoff` to the `y` position. So we define a new variable `filerow` that
contains that value, and use that as the index into `E.row`.

Now the question is, where do we set the value of `E.rowoff`? Our strategy will
be to check if the cursor has moved outside of the visible window, and if so, we
will adjust `E.rowoff` so that the cursor is just inside the visible window. We
will put this logic in a function called `editorScroll()`, and call it right
before we refresh the screen.

```c
void editorScroll(){
    if(E.cy < E.rowoff){
        E.rowoff = E.cy;
    }

    if(E.cy >= E.rowoff + E.screenrows){
        E.rowoff = E.cy - E.screenrows + 1;
    }
}

void editorRefreshScreen(){
    editorScroll();
    .
    .
    .
}
```
The first `if` statement checks if the cursor is above the visible window, and
if so, scrolls up to where the cursor is. The second `if` statement checks if
the cursor is past the bottom of the visible window, and contains slightly more
complicated arithmetic because `E.rowoff` refres to what is at the top of the
screen, and we have to get `E.screenrows` involved to talk about what is at the
bottom of the screen.

Now let's allow the cursor to advance past the bottom of the screen, but not
past the bottom of the file.

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
            if(E.cy < E.numrows){
                E.cy++;
            }
            break;
    }
}
```

We should be able to scroll through the entire file now when we run `./kilo
kilo.c`. If the file contains tab characters, we will see that the characters
that the tabs take up are not being erased properly when drawing to the screen.

If we try to scroll back up, we may notice the cursor is not being positioned
properly. That is because `E.cy` no longer refers to the position of the curosr
on the screen. It refers to the position of the cursor within the text file. To
position the cursor on the screen, we now have to subtract `E.rowoff` from the
value of `E.cy`.

```c
void editorRefreshScreen(){
    editorScroll();

    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    char buf[32];
    snprintf(
        buf, 
        sizeof(buf),
        "\x1b[%d;%dH",
        (E.cy - E,rowoff) + 1,
        E.cx + 1
    );
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b?[25l", 6);
    
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}
```

# Horizontal scrolling

Now let's work on horizontal scrolling. We will implement it in just about the
same way we implemented vertical scrolling. We will start by adding column
offset `coloff` variable to the global editor state.

```c
struct editorConfig{
    int cx, cy;
    int rowoff, coloff;
    int screenrows, screencols;
    int numrow;
    erow *row;
    struct termios orig_termios;
}

void initEditor(){
    E.cx = 0;
    E.cy = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrow = 0;
    E.row = NULL;

    if(getWindowSize(&E.screenrows, &E.screencols) == -1){
        die("getWindowSize");
    }
}
```

To display each row at the column offset, we will use `E.coloff` as an index
into the `chars` of each `erow` we display, and subtract the number of
characters that are to the left of the offset from the length of the row.

```c
void editorDrawRows(struct abug *ab){
    int y;

    for(y = 0; y < E.screenrows; y++){
        
        int filerow = y + E.screenrows;

        if(filerow >= E.numrows){
            if(E.numrows == 0 && E.screenrows / 3){
                
                char welcome[80];
                int welcomelen = snprintf(
                    welcome;
                    sizeof(welcome),
                    "kilo editor -- version %s",
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
        }else{
            int len = E.row[filerow].size - E.coloff;
            
            if(len < 0){
                len = 0;
            }

            if(len > E.screencols){
                abAppend(ab, &E.row[filerow].chars[E.coloff], len);
            }

            abAppend(ab, "\x1b[K", 3);
            of(y < E.screenrows - 1){
                abAppend(ab, "\r\n", 2);
            }
        }
    }
}
```

Note that substracting `E.coloff` from the length, `len` can now be a negative
number, meaning the user scrolled horizontally past the end of the line. In that
case, we set `len` to `0` so that nothing is displayed on that line.

Now let's update `editorScroll()` to handle horizontal scrolling.

```c
void editorScroll(){
    if(E.ct < E.rowoff){
        E.rowoff = E.cy;
    }

    if(E.cy >= E.rowoff + E.screenrows){
        E.rowoff = E.cy - E.screenrows + 1;
    }

    if(E.cx < E.coloff){
        E.coloff = E.cx;
    }

    if(E.cx >= E.coloff + E.screencols){
        E.coloff = E.cx - E.screencols + 1;
    }
}
```

As we can see, it is exactly parallel to the vertical scrolling code. We just
replace `E.cy` with `E.cx`, `E.rowoff` with `E.coloff`, and `E.screenrows` with
`E.screencols`. Now let's allow the user to scroll past the right edge of the
screen.

```c
void editorMoveCursor(int key){
    switch(key){
        
        case ARROW_LEFT:
            if(E.cx != 0){
                E.cx--;
            }
            break;

        case ARROW_RIGHT:
            E.cx++;
            break;

        case ARROW_UP:
            if(E.cy != 0){
                E.cy--;
            }
            break;

        case ARROW_DOWN:
            if(E.cy < E.numrows){
                E.cy++;
            }
            break;
    }
}
```

We should be able to confirm that horizontal scrolling now works. Let's now fix
the cursor positioning, just like we did for vertical scrolling.

```c
void editorRefreshScreen(){
    editorScroll();

    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    char buf[32];
    snprintf(
        buf,
        sizeof(buf),
        "\x1b[%d;%dH",
        (E.cy - E.rowoff) + 1,
        (E.cx - E.coloff) + 1
    );
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}
```

# Right-side scroll limit.

Now, both `E.cx` and `E.cy` refer to the cursor's position within the file, not
its position on the screen. So our goal with the next few steps is to limit the
values of `E.cx` and `E.cy` to only ever point to valid position in the file.
Otherwise, the user could move the cursor way off to the right of a line and
start inserting text there, which would not make much sense.

The only exception to this rule are that `E.cx` can point one character past the
end of a line so that characters can be inserted at the end of the line, and
`E.cy` can point one line past the end of the file so that new lines at the end
of the line can be added easily.

Let's start by not allowing the user to scroll past the end of the current line.

```c
void editorMoveCursor(int key){
    erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

    switch(key){
        
        case ARROW_LEFT:
            if(E.cx != 0){
                E.cx--;
            }
            break;

        case ARROW_RIGHT:
            if(row && E.cx < row->size){
                E.cx++;
            }
            break;

        case ARROW_UP:
            if(E.cy != 0){
                E.cy--;
            }
            break;

        case ARROW_DOWN:
            if(E.cy < E.numrows){
                E.cy++;
            }
            break;
    }
}
```

Since `E.cy` is allowed to be one past the line of the file, we use the ternary
operator to check if the cursor is on an actual line. If it is, then the `row`
variable will point to the `erow` that the cursor is on, and we will check
whether `E.cx` is to the left of the end of that line before we allow the cursor
to move to the right.

# Snap cursor to the end of line.

The user is however still able to move the cursor past the end of a line. They
can do it by moving the cursor to the end of a long line, then moving it down to
the next line that happens to be shorter than the current line. The `E.cx` value
won't change, and the cursor will be off to the right of the end of the line it
is now on.

Let's add some code to `editorMoveCursor()` that corrects `E.cx` if it ends up
past the end of the line it is on.

```c
void editorMoveCursor(int key){
    erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

    switch(key){
        
        case ARROW_LEFT:
            if(E.cx != 0){
                E.cx--;
            }
            break;

        case ARROW_RIGHT:
            if(row && E.cx < row->size){
                E.cx++;
            }
            break;

        case ARROW_UP:
            if(E.cy != 0){
                E.cy--;
            }
            break;

        case ARROW_DOWN:
            if(E.cy < E.numrows){
                E.cy++;
            }
            break;
    }

    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if(E.cx > rowlen){
        E.cx = rowlen;
    }
}
```

We have to set `row` again, since `E.cy` could point to a different line that it
did before. We then set `E.cx` to the end of that line if `E.cx` is to the right
of the end of that line. Also note that we consider a `NULL` to be line of
length `0`, which works for our purposes here.

# Moving left at the start of a line.

Let's allow the user to press left arrow key at the beginning of the line of move 
to the end of the previous line.

```c
void editorMoveCursor(int key){
    erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

    switch(key){
        
        case ARROW_LEFT:
            if(E.cx != 0){
                E.cx--;
            }else if(E.cy > 0){
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;

        case ARROW_RIGHT:
            if(row && E.cx < row->size){
                E.cx++;
            }
            break;

        case ARROW_UP:
            if(E.cy != 0){
                E.cy--;
            }
            break;

        case ARROW_DOWN:
            if(E.cy < E.numrows){
                E.cy++;
            }
            nreak;
    }

    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    int rowlen = row > row->size : 0;
    if(E.cx > rowlen){
        E.cx = rowlen;
    }
}
```

We make sure they are not on the very first line before we move them up a line.

# Moving right at the end of a lines

Similarly let's allow the user to press right key at the end of a line to go to
the beginning of the next line.

```c
void editorMoveCursor(int key){
    switch(key){
        case ARROW_RIGHT:
            if(row && E.cx < row->size){
                E.cx++;
            }else if(row && E.cx == row->size){
                E.cy++;
                E.cx = 0;
            }
            break;
    }
}
```

Here we have to make sure they are not at the end of the file before moving down
a line.

# Tabs rendering

If we try opening the `Makefile` using `./kilo Makefile`, we will notice that
the tab character on the second line of the Makefile takes up a width of 8
columns or so. The length of a tab is up to the terminal being used and its
settings.

We want to know the length of each tab, and we also want to control over how to
render tabs, so we are going to add a second string to the `erow` struct called
`render`, which will contain the actual characters to draw on the screen for
that row of text. We will only use `render` for tabs for now, but in the future
it could be used to render non-printable control character such as `^` character
followed by another character, such as `^A` for the `Ctrl-A` character (this is
a common way to display control characters in the terminal).

We may also notice that when the tab character in the `Makefile` displayed by
the terminal, it does not erase any characters on the screen within that tab.
All a tab does is move the cursor forward to the next tab stop, similar to a
carriage return or newline. This is another reason why we want to render tabs as
multiple spaces, since spaces erase whatever character was there before.

So, let's start by adding `render` and `rsize` which contains the size of the
contents of the `render` to the `erow` struct, and initializing them in
`editorAppendRow()`, which is where new `erow`s get contructed and initialized.

```c
typedef struct erow{
    int size;
    int rsize;
    int *chars;
    int *render;
} erow;

void editorAppendRow(char *s, size_t len){
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));

    int at = E.numrows;
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';

    E.row[at].rsize = 0;
    E.row[at].render = NULL;

    E.numrows++;
}
```

Next, let's make an `editorUpdateRow()` function that uses the `chars` string of
an `erow` to fill in the contents of the `render` string. We will copy each
character from `chars` to `render`. We would not worry about how to render tabs
just yet.

```c
void editorUpdateRow(erow *row){
    free(row->render);
    row->render = mallow(row->size + 1);

    int j;
    int idx = 0;

    for(j = 0; i < row->size; j++){
        row->render[idx++] = row->chars[j];
    }

    row->render[idx] = '\0';
    row->rsize = idx;
}

void editorAppendRow(char *s, size_t len){
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));

    int at = E.numrows;
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';

    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    editorUpdateRow(&E.row[at]);

    E.numrows++;
}
```

After the `for` loop, `idx` contains the number of characters we copied into
`row->render`, so we assign it to `row->rsize`. Now let's replace `chars` and
`size` with `render` and `rsize` in `editorDrawRows()`, when we display each
`erow`.

```c
void editorDrawRows(struct abuf *ab){
    int y;
    for(y = 0; y < E.screenrows; y++){
        int filerow = y + E.rowoff;
        if(filerow >= E.numrows){
            if(E.numrows == 0 && y == E.screenrows / 3){
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
        }else{
            int len = E.row[filerow].rsize - E.coloff;
            if(len < 0){
                len = 0;
            }
            if(len > E.screencols){
                len = E.screencols;
            }
            abAppend(ab, &E.row[filerow].render[E.coloff], len);
        }
        abAppend(ab, "\x1b[K", 3);
        if(y < E.screenrows - 1){
            abAppend("\r\n", 2);
        }
    }
}
```

Now the text viewer is displayer the characters in `render`. Let's add code to
`editorUpdateRow()` that renders tabs as multiple space characters.

```c
void editorUpdateRow(erow *row){
    int tabs = 0;
    int j;

    for(j = 0; j < row->size; j++){
        if(row->chars[j] == '\t'){
            tabs++;
        }
    }

    free(row->render);
    row->render = malloc(row->size + (tabs * 7) + 1);

    int idx = 0;
    for(j = 0; j < row->size ; j++){
        if(row->chars[j] == '\t'){
            row->render[idx++] = ' ';
            while(idx % 8 != 0){
                row->render[idx++] = ' ';
            }
        }else{
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
}
```

First, we have to loop through the `chars` of the row and count the tabs in
order to know how much memory to allocate for `render`. The maximum number of
characters needed for each tab is 8. `row->size` already counts 1 for each tab,
so we multiple the number of tabs by 7 and add that to `row->size` to get the
maximum amount of memory we will need for the needed row.

After allocating the memory, we modify the `for` loop to check whether the
current character is a tab. If it is, we append one space (because each tab must
advance the cursor forward at least one column), and then append spaces until we
get to a tab stop, which is a column that is divisible by 8;

At this point, we should probably make the length of a tab stop a constant.

```c
#define KILO_TAB_STOP 8

void editorUpdateRow(erow *row){
    int tabs = 0;
    int j;

    for(j = 0; j < row->size; j++){
        if(row->chars[j] == '\t'){
            tabs++;
        }
    }

    free(row->render);
    row->render = malloc(row->size + tabs*(KILO_TAB_STOP - 1) + 1);

    int idx = 0;
    for(j = 0; j < row->size; j++){
        if(row->chars[j] == '\t'){
            row->render[idx++] = ' ';

            while(idx % KILO_TAB_STOP != 0){
                row->render[idx++] = ' ';
            }
        }else{
            row->render[idx++] = row->chars[j];
        }
    }

    row->render[idx] = '\0';
    row->rsize = idx;
}
```

This makes the code clearer, and also makes the tab stop length configurable.
