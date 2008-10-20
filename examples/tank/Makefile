SVGS =	brick.svg bullet.svg bush.svg mud.svg rock.svg \
	tank1.svg tank2.svg tank_en.svg bang.svg
SVGHS =	$(SVGS:C/\.svg/.h/)
SVGCS =	$(SVGS:C/\.svg/.c/)
SVGOS = $(SVGS:C/\.svg/.o/)
MBS =	$(SVGS:C/\.svg/.mb/)
SVG2CODE=	../../tools/svg2code.py
MB_C_HEADER=    ../../tools/mb_c_header.m4
M4 =	m4
M4FLAGS ?=	-I /usr/local/share/mb 
LDFLAGS +=	
LIBS +=		-lmbfly -lX11 -L/usr/local/lib `pkg-config --libs cairo`
CFLAGS +=	-I/usr/local/include `pkg-config --cflags cairo` -I../../src
BIN = tank

all:	tank

tank:	tank_main.o $(SVGOS)
	$(CC) $(LDFLAGS) -o $@ $(.ALLSRC) $(LIBS)

tank_main.o: tank_main.c svgs.h
	$(CC) $(CFLAGS) -c tank_main.c

svgs.h: $(SVGHS)
	echo "#ifndef __SVGS_H_" > $@
	echo "#define __SVGS_H_" >> $@
	echo >> $@
	for H in $(SVGHS); do \
		echo "#include \"$$H\"" >> $@; \
	done
	echo >> $@
	echo "#endif /* __SVGS_H_ */" >> $@

.for SVG in $(SVGS)
$(SVG:C/\.svg/.o/): $(SVG:C/.svg/.c/)
	$(CC) $(CFLAGS) -c -o $@ $(.ALLSRC)

$(SVG:C/\.svg/.mb/): $(SVG)
	$(SVG2CODE) $(SVG) $@

$(SVG:C/\.svg/.h/):	$(SVG:C/\.svg/.mb/)
	$(M4) $(M4FLAGS) $(MB_C_HEADER) $(.ALLSRC) > $@

$(SVG:C/\.svg/.c/):	$(SVG:C/\.svg/.mb/)
	$(M4) $(M4FLAGS) $(MB_C_HEADER) $(.ALLSRC) > $@

.endfor

clean:
	@for f in svgs.h $(SVGHS) $(SVGCS) $(SVGOS) $(MBS) \
		*~ *.core tank tank_main.o; do \
		if [ -e $$f ]; then \
			echo "delete $$f"; rm -f $$f; \
		fi; \
	done
