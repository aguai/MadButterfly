SVG=svg2code_ex.svg
PREFIX?=/usr/local
TOOLSDIR?=$(PREFIX)/share/mb
INCDIR?=$(PREFIX)/include
LIBDIR?=$(PREFIX)/lib
BINDIR?=$(PREFIX)/bin
INCS=-I$(INCDIR)
CFLAGS+=`pkg-config --cflags cairo` $(INCS) -Wall
LDFLAGS=-L$(LIBDIR) `pkg-config --libs cairo`
LIBS=-lmbfly
BINS=	ex1
SVG2CODE=	$(BINDIR)/svg2code.py
MB_C_HEADER=    $(TOOLSDIR)/mb_c_header.m4
MB_C_SOURCE=    $(TOOLSDIR)/mb_c_source.m4

all:	$(BINS)

ex1:	main.o $(SVG:C/.svg/.o/)
	$(CC) $(LDFLAGS) -o $@ $(.ALLSRC) $(LIBS)

main.o:	main.c $(SVG:C/.svg/.h/)
	$(CC) $(CFLAGS) -c -o $@ main.c

$(SVG:C/.svg/.o/): $(SVG:C/.svg/.c/) $(SVG:C/.svg/.h/)
	$(CC) -c $(CFLAGS) -o $@ $(SVG:C/.svg/.c/)

$(SVG:C/.svg/.mb/): $(SVG)
	$(SVG2CODE) $(.ALLSRC) $@

$(SVG:C/.svg/.c/): $(SVG:C/.svg/.mb/)
	m4 -I $(TOOLSDIR) $(MB_C_SOURCE) $(.ALLSRC) > $@

$(SVG:C/.svg/.h/): $(SVG:C/.svg/.mb/)
	m4 -I $(TOOLSDIR) $(MB_C_HEADER) $(.ALLSRC) > $@

clean:
	for i in *.mb *.o *.core *~ $(SVG:C/.svg/.c/) $(SVG:C/.svg/.h/) $(BINS); do \
		if [ -e "$$i" ]; then \
			echo "delete $$i"; \
			rm -f "$$i"; \
		fi; \
	done
