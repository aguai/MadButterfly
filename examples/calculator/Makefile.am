include $(top_srcdir)/config.mk

noinst_PROGRAMS = calc
EXTRA_DIST = calculator_scr.svg

calc_SOURCES = main.c
nodist_calc_SOURCES = calculator_scr.c calculator_scr.h
calc_CPPFLAGS = $(APPCFLAGS) $(INCLUDES)
calc_LDFLAGS = $(APPLDFLAGS)
calc_LDADD = $(top_builddir)/src/libmbfly.la
BUILT_SOURCES = calculator_scr.c calculator_scr.h calculator_scr.mb
CLEANFILES = calculator_scr.c calculator_scr.h calculator_scr.mb

calculator_scr.mb: $(srcdir)/calculator_scr.svg
	$(top_srcdir)/tools/svg2code.py -s $? $@

calculator_scr.h: calculator_scr.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

calculator_scr.c: calculator_scr.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@
