
mbfly_node_SRCS = mbfly_njs.cc X_supp_njs.c coord.cc shapes.cc paints.cc \
		font.cc
mbfly_node_CFLAGS= -I$(abs_top_builddir)/include \
	-I$(abs_top_srcdir)/include \
	-I$(prefix)/include \
	@pangocairo_CFLAGS@ $(CFLAGS)
mbfly_node_LDFLAGS = -L$(abs_top_builddir)/src/.libs @pangocairo_LIBS@

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
	cd $(srcdir); \
	WAFLOCK=$(abs_builddir)/objs/.lock-wscript \
		$(NODE_WAF) clean
