SVG=calculator_scr.svg
TOOLSDIR=/usr/local/share/mb
INCS=-I/usr/local/include
CFLAGS+=`pkg-config --cflags cairo` $(INCS) -Wall -I../../src
LDFLAGS=-L/usr/local/lib `pkg-config --libs cairo` -L../../src
LIBS=-lmbfly
BINS=	calc
SVG2CODE=	../../tools/svg2code.py
MB_C_HEADER=    ../../tools/mb_c_header.m4
MB_C_SOURCE=    ../../tools/mb_c_source.m4

all:	$(BINS)

calc:	main.o $(SVG:C/.svg/.o/)
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
