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


