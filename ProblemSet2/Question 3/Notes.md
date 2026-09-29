This program will NOT run compile on windows. This was created with compiling utilizing UNIX systems in mind. Tested with WSL and cygwin64

Cygwin64 will produce more accurate behavior, WSL will not create the no perm error consistently.

to test use:

make
make test

to clean testarea:
make clean