SRCS =	coord.c geo.c shape_path.c shape_text.c shape_rect.c \
	redraw_man.c timer.c animate.c paint.c event.c observer.c \
	X_supp.c timertool.c tools.c shift.c chgcolor.c \
	visibility.c
OBJS = ${SRCS:C/(.*)\.c/\1.o/g}
TESTCASE_OBJS = ${SRCS:C/(.*)\.c/testcase-\1.o/g}
CFLAGS+=	-Wall -I/usr/local/include `pkg-config --cflags cairo`
LDFLAGS =	`pkg-config --libs cairo`
BINS =	libmbfly.a X_main
PREFIX?=/usr/local/
HEADERS=	X_supp.h animate.h mb.h mb_timer.h mb_types.h \
		observer.h paint.h redraw_man.h shapes.h tools.h
MKDIR?=	mkdir -p

all: $(BINS)

install: $(BINS)
	$(INSTALL) -d ${PREFIX}include/mb
.for i in $(HEADERS)
	$(INSTALL) -o root -m 0644 ${i} ${PREFIX}include/mb/
.endfor
	$(INSTALL) -o root libmbfly.a ${PREFIX}lib/

uninstall:
.for i in $(HEADERS)
	rm -f ${PREFIX}include/mb/${i}
.endfor
	rm -f ${PREFIX}lib/libmbfly.a
	rmdir ${PREFIX}include/mb/

testcase: testcase.o $(TESTCASE_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(.ALLSRC) -L/usr/local/lib -lcunit

libmbfly.a: $(OBJS)
	$(AR) -cr $@ $(.ALLSRC)


.for i in $(TESTCASE_OBJS)
${i}: ${i:C/testcase-(.*).o/\1.c/}
	$(CC) $(CFLAGS) -DUNITTEST -g -c -o $@ $(.ALLSRC)
.endfor

testcase.o:	testcase.c
	$(CC) $(CFLAGS) -c $(.ALLSRC)

X_main: X_main.o libmbfly.a
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
