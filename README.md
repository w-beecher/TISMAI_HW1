# Description
This is a "compiler" I wrote a few years ago now for an introductory compilers course at my Alma Mater.  The purpose is to convert Rust source files into an executable.  This setup uses Flex for the lexing and Bison for the parsing.  Due to time constraints in the course, this does not compile directly to assembly from intermediate code, but instead transpiles into assembly-like C code and invokes GCC (A future personal project is to actually implement the assembly portion, which is really the interesting part of a compiler anyways). Similarly, due to course restrictions, this only supports a subset of Rust (which was originally defined in a vague specification that I no longer have access to, for which I apologize).

I do still have a number of example test files which are provided, which do at least show functionality on the aspects that were included.  A large portion of this work was adding in semi-informative error messages; you are free to mess around with source files to see what you can find, though without the languague specification this might produce some really strange results or errors in some cases.

# Setup
I have provided a Dockerfile to create a virtual enviornment, which should ensure that everything is set up properly automatically (Assuming you're running Ubuntu).


The image can be created via `sudo docker build -t hw-1-image .`, or whatever other name you'd like instead of hw-1-image.  This should take roughly a minute or two.

This can then be run via `sudo docker run -it hw-1-image`.  

When finished, you can remove the image however you'd like.

# Running & Usage
The Dockerfile will automatically run the makefile, so all compilation should be completed already.  The built executable is `fec`.  You are free to use the makefile yourself to `make clean` or re`make` if you wish.

To see what it's doing, I have included a `tests/` directory which contains a number of example run files illustrating a variety of features (mostly to cover functionality on the original rubric of this assignment).  Executables are always placed in the same directory as the source files, NOT in the current working directory.  Finally, be aware that some files are testing features that are not immediately obvious in the title: for example, `flow control/if.rs` also contains a test for boolean short-circuiting by including a divide-by-zero operation in the if statement.  I would reccomend reading these test files to ensure that the output matches what one would expect; they are very very simple.

Usage: `./fec [executable]`.  For example, in the root directory, `./fec tests/builtins/read.rs` and the executable can be run via `./tests/builtins/read`.

Finally, this supports a number of command line argument tags.  The ones that may be of interest are:
- `-icode` which outputs to stdout a representation of assembly-like intermediate code
- `-c` which compiles into an object file (i.e., skips the linking step)
- `-s` which retains a TAC.c representation of the intermediate code