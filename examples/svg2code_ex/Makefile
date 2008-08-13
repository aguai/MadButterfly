SVG=svg2code_ex.svg
TOOLSDIR=../../tools

all: $(SVG:C/.svg/.mb/) $(SVG:C/.svg/.c/) $(SVG:C/.svg/.h/)

$(SVG:C/.svg/.mb/): $(SVG)
	$(TOOLSDIR)/svg2code.py $(.ALLSRC) $@
	
$(SVG:C/.svg/.c/): $(SVG:C/.svg/.mb/)
	m4 -I $(TOOLSDIR) mb_c_source.m4 $(.ALLSRC) > $@

$(SVG:C/.svg/.h/): $(SVG:C/.svg/.mb/)
	m4 -I $(TOOLSDIR) mb_c_header.m4 $(.ALLSRC) > $@

clean:
	for i in *.mb *.o *~; do \
		if [ -e "$$i" ]; then \
			echo "delete $$i"; \
			rm -f "$$i"; \
		fi; \
	done