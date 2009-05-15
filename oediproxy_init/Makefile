CFLAGS=-Wall -pedantic -g -fprofile-arcs -ftest-coverage
CLEAN_STUFF=oscar2test *.o *~ *.gc*

all: check

oscar2test: oscar2test.c oscar2.c oscar2.h
	$(CC) $(CFLAGS) oscar2test.c oscar2.c -o oscar2test

check: oscar2test
	./oscar2test

coverage: oscar2test
	./oscar2test
	gcov oscar2test.c oscar2.c

clean:
	rm -f $(CLEAN_STUFF)
