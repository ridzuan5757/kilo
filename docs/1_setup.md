# `main()` function

In C, all of the executable code has to be placed inside function. The `main()`
function in C is special. It is the default starting point when the program is
running. When we `return` from `main()` function, the programs exits and passes
the returned integer back to the operating system. Return value of `0` indicates
success.

```c
// kilo.c

int main(void){
    return 0;
}
```

C is a compiled language. That means we need to run the program to C compiler to
turn it into executable file. The executable file can be run as any other
program via the terminal.

To compile `kilo.c`, run `cc kilo.c -o kilo` via the shell. If no errors occur,
this will produce an executable file named `kilo`. `-o` stands for output and
specifies that the output executable should be named `kilo`.

To run `kilo`, type `./kilo` in the shell and press `Enter`. The program does
not print any output, but the exit status can be checked by running `echo $?`,
which should print `0`.

# `Makefile`

Compiling the project by typing in the terminal `cc kilo.c -o kilo` can be
repititive. The `make` program allows us to run `make`, and it will compile the
program altogether. We just have to supply a `Makefile` to tell it how the
program should be compiled.

```Makefile
kilo:   kilo.c
    $(CC) kilo.c -o kilo -Wall -Wextra -pedantic -std=99

```
