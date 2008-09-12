SVG=svg2code_ex.svg
TOOLSDIR=../../tools
INCS=-I/usr/local/include
CFLAGS+=`pkg-config --cflags cairo` $(INCS) -Wall
LDFLAGS=-L/usr/local/lib `pkg-config --libs cairo`
LIBS=-lmbfly
BINS=	ex1

all:	$(BINS)

ex1:	main.o $(SVG:C/.svg/.o/)
	$(CC) $(LDFLAGS) -o $@ $(.ALLSRC) $(LIBS)

main.o:	main.c $(SVG:C/.svg/.h/)
	$(CC) $(CFLAGS) -c -o $@ main.c

$(SVG:C/.svg/.o/): $(SVG:C/.svg/.c/) $(SVG:C/.svg/.h/)
	$(CC) -c $(CFLAGS) -o $@ $(SVG:C/.svg/.c/)

$(SVG:C/.svg/.mb/): $(SVG)
	$(TOOLSDIR)/svg2code.py $(.ALLSRC) $@

$(SVG:C/.svg/.c/): $(SVG:C/.svg/.mb/)
	m4 -I $(TOOLSDIR) mb_c_source.m4 $(.ALLSRC) > $@

$(SVG:C/.svg/.h/): $(SVG:C/.svg/.mb/)
	m4 -I $(TOOLSDIR) mb_c_header.m4 $(.ALLSRC) > $@

clean:
	for i in *.mb *.o *.core *~ $(SVG:C/.svg/.c/) $(SVG:C/.svg/.h/) $(BINS); do \
		if [ -e "$$i" ]; then \
			echo "delete $$i"; \
			rm -f "$$i"; \
		fi; \
	done