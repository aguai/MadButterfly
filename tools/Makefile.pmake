BINS= svg2code.py
M4SCRIPTS= mb_c_header.m4 mb_c_source.m4 foreach.m4
PREFIX?= /usr/local/

all:

install:
.for i in $(BINS)
	$(INSTALL) -m 755 ${i} ${PREFIX}bin/
.endfor
	$(INSTALL) -d ${PREFIX}share/mb
.for i in $(M4SCRIPTS)
	$(INSTALL) ${i} ${PREFIX}share/mb/
.endfor

clean:
	for i in *~; do \
		echo "delete $$i"; rm -f $$i; \
	done
