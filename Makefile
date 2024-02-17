# Compiling with make
# The make program allow us to simply run `make` and it will compile the 
# program. We just have to supply the `Makefile` to tell it how to compile the
# program. 

# The first line says that `kilo` is what we want to build, and that kilo.cc
# is what is required to build it.
# The second line specifies the command to run in order to actually build 
# `kilo` out of `kilo.c`.
#
# Make sure to indent the second line with an actual tab character, and not 
# with spaces.
# 	-	C files can be indented however we want, but `Makefile`s must use tabs.
kilo: kilo.c
	$(CC) kilo.c -o kilo -Wall -Wextra -pedantic -std=c99

# We have added a few things to the compilation command:
# 	-	`$(CC)` is a variable that `make` expands to `cc` by default.
# 	- `-Wall` stands for "all warnings", and gets the compiler to warn us when
# 	it seees code in the program that might not be technically wrong, but is
# 	considered bad or questionable usage of the C language, like using variables
# 	before initializing them.
# 	- `-Wextra` and `-pedantic` turn on even more warnings. For each step in
# 	this program, if the program compiles, it should not produce any warnings
# 	except for "unused variable" warning in some cases. If other warnings is
# 	produced, check to make sure the code is appropriate.
# 	- `-std=c99` specifies the exact version of the C language that is being
# 	used which is c99. C99 allows us to declare variables anywhere within a
# 	function, whereas ANSI C requires all variables to be declared at the top of
# 	a function or block.
