all: hunt
 
hunt: hunt.c
	gcc -o hunt hunt.c
 
test: hunt
	chmod +x test.sh
	./test.sh
	./hunt testarea/target testarea
 
clean:
	chmod -R 755 testarea 2>/dev/null || true
	rm -rf hunt testarea