
mbfly_node_SRCS = hello.cc

all: mbfly.node
clean: clean-mbfly-node

mbfly.node: wscript $(mbfly_node_SRCS)
	cd $(srcdir); \
	WAFLOCK=$(abs_builddir)/.lock-wscript $(NODE_WAF) \
		configure build --blddir=$(abs_builddir)

clean-mbfly-node:
	$(NODE_WAF) clean
