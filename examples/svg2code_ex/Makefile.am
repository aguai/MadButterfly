include $(top_srcdir)/config.mk

noinst_PROGRAMS = ex1
EXTRA_DIST = svg2code_ex.svg dsc_3241.png

ex1_SOURCES = main.c
nodist_ex1_SOURCES = svg2code_ex.c svg2code_ex.h
ex1_CPPFLAGS = @pangocairo_CFLAGS@ -I$(top_srcdir)
ex1_LDFLAGS = @pangocairo_LIBS@ 
ex1_LDADD = $(top_builddir)/src/libmbfly.la
BUILT_SOURCES = svg2code_ex.c svg2code_ex.h svg2code_ex.mb
CLEANFILES = svg2code_ex.c svg2code_ex.h svg2code_ex.mb

svg2code_ex.mb: $(srcdir)/svg2code_ex.svg
	$(top_srcdir)/tools/svg2code.py $? $@

svg2code_ex.h: svg2code_ex.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

svg2code_ex.c: svg2code_ex.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@
