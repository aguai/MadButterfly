include $(top_srcdir)/config.mk

noinst_PROGRAMS = tank
#EXTRA_DIST = 
svg_sources = brick.c bullet.c bush.c mud.c rock.c \
	tank1.c tank2.c tank_en.c bang.c
tank_SOURCES = tank_main.c
nodist_tank_SOURCES = svgs.h \
	$(svg_sources) $(svg_sources:.c=.h) $(svg_sources:.c=.mb)
tank_CPPFLAGS = @pangocairo_CFLAGS@
tank_LDFLAGS = @pangocairo_LIBS@ 
tank_LDADD = $(top_builddir)/src/libmbfly.la
BUILT_SOURCES = svgs.h \
	$(svg_sources) $(svg_sources:.c=.h) $(svg_sources:.c=.mb)
CLEANFILES = svgs.h \
	$(svg_sources) $(svg_sources:.c=.h) $(svg_sources:.c=.mb)

$(svg_sources): %.c: %.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@

$(svg_sources:.c=.h): %.h: %.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

$(svg_sources:.c=.mb): %.mb: $(srcdir)/%.svg
	$(top_srcdir)/tools/svg2code.py $? $@

svgs.h: $(svg_sources:.c=.h)
	echo "#ifndef __SVGS_H_" > $@
	echo "#define __SVGS_H_" >> $@
	echo >> $@
	for H in $(svg_sources:.c=.h); do \
		echo "#include \"$$H\"" >> $@; \
	done
	echo >> $@
	echo "#endif /* __SVGS_H_ */" >> $@
