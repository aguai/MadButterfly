SRCS =	coord.c
TESTCASE_OBJS = ${SRCS:C/(.*)\.c/testcase-\1.o/g}
CFLAGS =	-I/usr/local/include
BINS =	testcase

all: $(BINS)

testcase: testcase.o $(TESTCASE_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(.ALLSRC) -L/usr/local/lib -lcunit

.for i in $(TESTCASE_OBJS)
${i}: ${i:C/testcase-(.*).o/\1.c/}
	$(CC) $(CFLAGS) -DUNITTEST -g -c -o $@ $(.ALLSRC)
.endfor

testcase.o:	testcase.c
	$(CC) $(CFLAGS) -c $(.ALLSRC)

clean:
	for i in *.o *~ *.core $(BINS); do \
		echo "delete $$i"; \
		rm -f $$i; \
	done
