#!./shell.o
pwd
echo redirdemo >out1.out
cat out1.out
echo appendedLine >>out1.out
cat out1.out

#above is an empty line which is skipped along with this commented line
ls -l 2>commandErrors.out
pwd
