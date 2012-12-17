#CFLAGS=-Wall -pedantic -g -fprofile-arcs -ftest-coverage
CFLAGS=-g -fprofile-arcs -ftest-coverage
CLEAN_STUFF=oscar2test ofttest *.o *~ *.gc*

all: check

ofttest: ofttest.c oft.c oscar2.c oscar2.h oft.h frame.c frame.h
	$(CC) $(CFLAGS) ofttest.c oft.c oscar2.c  frame.c -o ofttest

oscar2test: oscar2test.c oscar2.c oscar2.h frame.c frame.h
	$(CC) $(CFLAGS) oscar2test.c oscar2.c frame.c -o oscar2test

check: oscar2test ofttest
	./oscar2test
	./ofttest

oscar2test_coverage: oscar2test
	./oscar2test
	gcov oscar2test.c oscar2.c


ofttest_coverage: ofttest
	./ofttest
	gcov ofttest.c oft.c

coverage: oscar2test_coverage ofttest_coverage

clean:
	rm -f $(CLEAN_STUFF)
