noinst_PROGRAMS = ex1
EXTRA_DIST = svg2code_ex.svg

ex1_SOURCES = main.c
nodist_ex1_SOURCES = svg2code_ex.c svg2code_ex.h
ex1_CPPFLAGS = @cairo_CFLAGS@ -I$(srcdir)
ex1_LDFLAGS = @cairo_LIBS@ 
ex1_LDADD = $(top_builddir)/src/libmbfly.la
BUILT_SOURCES = svg2code_ex.c svg2code_ex.h svg2code_ex.mb
CLEANFILES = svg2code_ex.c svg2code_ex.h svg2code_ex.mb

svg2code_ex.mb: $(srcdir)/svg2code_ex.svg
	$(top_srcdir)/tools/svg2code.py $? $@

svg2code_ex.h: svg2code_ex.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

svg2code_ex.c: svg2code_ex.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@
