SRCS =	coord.c geo.c shape_path.c shape_text.c shape_rect.c \
	redraw_man.c timer.c animate.c paint.c event.c observer.c \
	X_supp.c tools.c
OBJS = ${SRCS:C/(.*)\.c/\1.o/g}
TESTCASE_OBJS = ${SRCS:C/(.*)\.c/testcase-\1.o/g}
CFLAGS+=	-Wall -I/usr/local/include `pkg-config --cflags cairo`
LDFLAGS =	`pkg-config --libs cairo`
BINS =	testcase X_main

all: $(BINS)

testcase: testcase.o $(TESTCASE_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(.ALLSRC) -L/usr/local/lib -lcunit

.for i in $(TESTCASE_OBJS)
${i}: ${i:C/testcase-(.*).o/\1.c/}
	$(CC) $(CFLAGS) -DUNITTEST -g -c -o $@ $(.ALLSRC)
.endfor

testcase.o:	testcase.c
	$(CC) $(CFLAGS) -c $(.ALLSRC)

X_main: X_main.o $(OBJS)
	$(CC) $(CFALGS) `pkg-config --libs cairo` -o $@ $(.ALLSRC)

X_main.o: X_main.c
	$(CC) $(CFLAGS) `pkg-config --cflags cairo` -c $(.ALLSRC)

clean:
	for i in *.o *~ *.core $(SHAPE_OBJS) $(BINS); do \
		echo "delete $$i"; \
		rm -f $$i; \
	done

depend:
	mkdep $(CFLAGS) $(SRCS)
