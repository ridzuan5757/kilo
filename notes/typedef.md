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


