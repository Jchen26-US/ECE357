intpipe: q3.c
	gcc q3.c -o intpipe
tests: intpipe
	-./intpipe 4096 10000
	-./intpipe 5550 10000
	-./intpipe 4096 10000 restart
	-./intpipe 5550 10000 restart
extraTests: intpipe
	-./intpipe 1 1
	-./intpipe 555 100
	-./intpipe 4095 1000
	-./intpipe 4097 10000
	-./intpipe 1 10000 restart
clean:
	rm intpipe
