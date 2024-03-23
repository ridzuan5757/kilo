# Typedef

`typedef` mecahnism allows the creation of aliases for other types. It does not
create new types. People often use `typedef` to improve the portability of code,
to give aliases to structure or union types, or to create aliases for function /
function pointer types.

In C standard, `typedef` is classified as storage class for convenience; it
occurs syntactically where storage classes such as static or extern could
appear.

## Typedef for Structure and Unions

```c
typedef struct Person{
    char name[32];
    int age;
} Person;

Person person;
```

Compared to the traditional way of declaring structs, programmers would not need
to have `struct` everytime they declare an instance of that struct.

Note that the name `Person` as opposed to `struct Person` is not defined until
the final semicolon. Thus for linked lists and tree structures which need to
contain a pointer to some structure type, we must use either:

```c
typedef struct Person{
    char name[32];
    int age;
    struct Person *next;
} Person;
```

Or:

```c
typedef struct Person Person;

struct Person{
    char name[32];
    int age;
    Person *next;
}
```

The use of a `typedef` for a `union` type is very similar:

```c
typedef union Float Float;

union Float{
    float f;
    char b[sizeof(floag)];
}

```

A structure similar to this can be used to analyze the bytes that make up a
`float` value.

## Simple Uses of Typedef

### Short name to data type.

Instead of:

```c
long long int foo;
struct mystructure object;
```

On can use:

```c
typedef long long ll;
typedef struct mystructure mystruct;

ll foo;
mystruct object;
```

This reduces the mount of typing needed if the type is used many times in the
program.

### Portability

The attirubtes of data types vary across different architectures. For example,
an int may be 2-byte in one implementation and 4-byte in another. Suppose a
program needs to use a 4-byte type to run correctly.

In one implementation, let the size of `int` to be 2 bytes and that of `long` to
be 4 bytes. In another, let the size of `int` to be 4 bytes and that of `long`
to be 8 bytes. If the program is written using the second implementation:

```c
// program expecting 4 byte integer
int foo;
```

For the program to run in the first implementation, all the `int` declarations
will have to change to `long`.

```c
long foo;
```

To avoud this, one can use `typedef`:

```c
// program expecting 4 byte integer
typedef int myint;
myint foo;
```

Then, only the `typedef` statement would need to be changed each time, instead
of examining the whole program. If set of data has a particular purpose, one 
can use `typedef` to give it a meaningful name. Moreover, if the property of the 
data changes such that the base type must change, only the `typedef` statement 
would have to be changed.

The `<stdint.h>` header and the related `<inttypes.h>` header define standard of
type names using `typedef` for integers of various sizes, and these names are
often the best choice in modern code that need fixed size integers.

For example `uint8_t` is an unssigned 8-bit integer, `int64_t` is a signed
64-bit integer type. The type `uintptr_t` is an unsigned integer type big enough
to hold any pointer to object.

These types are theoretically optional, byt it is rare for them to not be
available. There are variants like `uint_least16_t` (smallest unsigned integer
type with at least 16 bits) and `int_fast32_t` (fastest signed integer type with
at least 32 bit). Also, `intmax_t` and `uintmax_t` are the largest integer types
supported by the implementation.

## Typedef for Function Pointers

We can use `typedef` to simplify the usage of function pointers. Image we have
some functions, all having the same signature, that use their argument to print
out in different ways:

```c
#include <stdio.h>

void print_to_n(int n){
    for(int i=1; i<-n; i++){
        printf("%d\n", i);
    }
}

void print_n(int n){
    printf("%d\n", n);
}
```

Now we can use a `typedef` to create a named function pointer type called
`printer`:

```c
typedef void(*printer_t)(int);
```

This creates a type, named `printer_t` for a pointer to a function that takes
single `int` argument and returns nothing, which matches the signature of the
functions `print_to_n` and `print_n`. To use it, we create a cariable of the
created type and ssign it a pointer to one of the functions in question:

```c
printer_t p = &print_to_n;

// this would be required without the type
void (*p)(int) = &print_to_n;
```

This the `typedef` allows a simpler syntax when dealing with function pointers.
This becomes more apparent when function pointers are used in more complex
situations, such as arguments to functions. 

If we are using a function that takes a function pointer as a parameter without
a function pointer type defined, the function definition would be:

```c
void foo(void (*printer)(int), int y){
    // code
    printer(y);
    // code
}
```

Likewie functions can return function pointers and again, the use of a `typedef`
can make the syntax simpler when doing so. A classic example is the `signal`
function from `<signal.h>`. The declaration for it is:

```c
void (*signal(int sig, void (*func)(int)))(int);
```

That is a function that takes two arguments:
- An `int`
- A pointer to a function that takes `int` as argument and returns `void`.

And this function returns a pointer to function like its second argument.

If we defined a type `SigCatcher` as an alias for the pointer to the function
time:

```c
typedef void (*SigCatcher)(int);
```

Then we could declare `signal()` using:

```c
SigCatcher signal(int sig, SigCatcher func);
```

On the whole, this is easier to uncerstand, even though the C standard did not
elect to define a type to do the job. The `signal` function takes two arguments,
and `int` and `SigCatcher`, and returns a `SigCatcher` is a pointer to a
function that taks an int argument and returns nothing.

Although using `typedef` names for pointer to function types makes life easier,
it can also lead to confusion for others who will maintain our code later on.
Make sure to use it with caution and proper documentation.

## Disadvantages of Typedef

`typedef` could lead to the namespace pollution in large C program.

## Disadvantages of Typedef Structs

`typedef`'d structs without a tag name a major caouse of needles imposition of
ordering relationship among header files. Consider:

```c
#ifndef FOO_H
#define FOO_H 1

#define FOO_DEF (0xDEADBABE)

// forward declaration, declared in bar.h
struct bar;

struct foo{
    struct bar* bar;
}

#endif
```

With such a definition, not using `typedefs`, it is possible for a compilation
unit to include `foo.h` to get at the `FOO_DEF` definition. If it does not
attempt to dereference the `bar` emmber of the `foo` struct then there will be
no need to include the `bar.h` file.

## Difference with #define
`#define` is a C pre-processor directive which also used to define the aliases
for various data types similar to `typedef` but with the following differences:
- `typedef` is limited to giving symbolic names to types only.
- `#define` can be used to define alias for values as well.
- `typedef` interpretation is performed by the compiler.
- `#define` statements are processed by pre-processor.
- Note that `#define cptr char*` followed by `cptr a, b` does not do the same as
  `typedef char *cptr` followed by `cptr a, b`. With `#define`, `b` is a plain
  `char` variable, but it is also a pointer with the `typedef`.
