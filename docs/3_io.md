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
