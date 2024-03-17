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

`mallow()` comes from `<stdlib.h>`. `ssize_t` comes from `<sys/types.h>`.
`editorOpen()` will eventually be for opening and reading file from disk. To
load our "Hello, world!" message into the editor's `erow` `struct`, we set the
`size` field to the length of our message, allocate the necessary memory using
`malloc()`, and `memcpy()` the message to the `chars` field which points to the
memory we allocated. Finally, we set the `E.numrows` variable to `1`, to
indicate that the `erow` now contains a line that should be displayed.
