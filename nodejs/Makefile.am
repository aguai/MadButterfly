# -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
# vim: sw=4:ts=8:sts=4

mbfly_node_SRCS = mbfly_njs.cc njs_mb_supp.c coord.cc shapes.cc paints.cc \
		font.cc
mbfly_node_CFLAGS= -I$(abs_top_builddir)/include \
	-I$(abs_top_srcdir)/include \
	-I$(prefix)/include \
	@imlib2_CFLAGS@ \
	@pangocairo_CFLAGS@ $(CFLAGS) $(CPPFLAGS)
mbfly_node_LDFLAGS = -L$(abs_top_builddir)/src/.libs @pangocairo_LIBS@ \
	@imlib2_LIBS@ $(LDFLAGS)

if OPENVG_GRAPH_ENGINE
mbfly_node_LDFLAGS += -lOpenVG
endif

if XSHM
mbfly_node_LDFLAGS += -lXext
endif

all: mbfly.node
clean: clean-mbfly-node

mbfly.node: wscript $(mbfly_node_SRCS)
	cd $(srcdir); \
	CFLAGS="$(mbfly_node_CFLAGS)" \
		CXXFLAGS="$(mbfly_node_CFLAGS)" \
		LDFLAGS="$(mbfly_node_LDFLAGS)" \
		TOP_BUILDDIR="$(abs_top_builddir)" \
		WAFLOCK=$(abs_builddir)/objs/.lock-wscript \
		$(NODE_WAF) configure build --srcdir=$(abs_srcdir) \
		--blddir=$(abs_builddir)/objs

clean-mbfly-node:
	-cd $(srcdir); \
	WAFLOCK=$(abs_builddir)/objs/.lock-wscript \
		$(NODE_WAF) clean
