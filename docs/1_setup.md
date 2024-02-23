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

The first line says that `kilo` is what we want to build, and that `kilo.c` is
what is required to build it. The second line specifies the command to run in
order to actually build `kilo` out of `kilo.c`. The second line has to be
indented with an actual tab character, not spaces. `Makefile` indentation has to
be via tabs.

Compilation command used in `Makefile`:
- `$(CC)` is a variable that `make` expands to `cc` by default.
- `-Wall` stands for **all warnings**, and gets the compiler to log warnings
  when it sees code in the program that might not technically be wrong, but is
  considered bad or questionable usage of the C language, such as using
  variables before initializing them.
- `Wextra` and `-pedantic` turn on more warnings that is not enabled by `-Wall`.
- `-std=99` specifies the exact version of the C language stadard we are using,
  which is `C99`. C99 allows us to declare variables anywhere within a function
  whereas `ANSI C` requires all variables to be declared at the top of a
  function or block.

Command `make` is used to compile the program using the `Makefile`. It may
output `make: 'kilo' is up to date.`. It can tell that the current version of
`kilo.c` has already been compiled by looking at each file's last modified
timestamp. If `kilo` modified after `kilo.c` was last modified, than `make`
assumes that `kilo.c` has already been compiled, and so it does not bother
running the compilation command. If `kilo.c` was last modified after `kilo` was,
then `make` will recompile `kilo.c`. This can be usefull on large projects with
many different components to compile, as most of the components should not need
to be recompiled over and over when we are only making changes to one
component's source code.

If we are changing the return value of `main()` to any number other than `0`,
run `make`, we should see the return value of `echo $?` will be the same as the
return value of `main()`.
