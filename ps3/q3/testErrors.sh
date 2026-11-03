#!./shell.o

cd ./nonExistentDirectory
cat ./nonExistentFile
nonExistentcommand
ls > noredirection
ls >multiredirect1.out > multiredirect2.out
echo > output.out extraarg
echo "test" > nonExistentDirectory/nonExistentFile.out
exit "stringExit"
echo "Shell is active post errors"
pwd
