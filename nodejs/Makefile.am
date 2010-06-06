
mbfly_node_SRCS = mbfly_njs.cc X_supp_njs.c
mbfly_node_CFLAGS= -I$(abs_top_builddir)/include \
	-I$(abs_top_srcdir)/include \
	-I$(prefix)/include \
	@pangocairo_CFLAGS@ $(CFLAGS)
mbfly_node_LDFLAGS = -L$(abs_top_builddir)/src/.libs @pangocairo_LIBS@

all: mbfly.node
clean: clean-mbfly-node

mbfly.node: wscript $(mbfly_node_SRCS)
	cd $(srcdir); \
	CFLAGS="$(mbfly_node_CFLAGS)" \
		LDFLAGS="$(mbfly_node_LDFLAGS)" \
		TOP_BUILDDIR="$(abs_top_builddir)" \
		WAFLOCK=$(abs_builddir)/.lock-wscript \
		$(NODE_WAF) configure build --blddir=$(abs_builddir)

clean-mbfly-node:
	cd $(srcdir); \
	WAFLOCK=$(abs_builddir)/.lock-wscript \
		$(NODE_WAF) clean
