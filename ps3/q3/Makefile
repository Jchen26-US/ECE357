shell: p3.c
	gcc p3.c -o shell.o
tests: shell.o test1.sh test2.sh testErrors.sh testRegular.sh
	-./test1.sh
	-make cleanOut
	-./test2.sh
	-make cleanOut
	-./testErrors.sh
	-make cleanOut
	-./testRegular.sh
	-make cleanOut
clean:
	rm *.out
	rm *.o
cleanOut:
	rm *.out
