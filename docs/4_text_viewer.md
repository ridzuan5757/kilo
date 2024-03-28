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

        }
    }
}
```
