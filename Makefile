SUBDIRS= src tools

all:
.for i in $(SUBDIRS)
	@echo "==> Enter subdirectory $(i)"
	@cd ${i}; make $@
	@echo "<== Leave subdirectory $(i)"
.endfor

install:
.for i in $(SUBDIRS)
	@echo "==> Enter subdirectory $(i)"
	@cd ${i}; make $@
	@echo "<== Leave subdirectory $(i)"
.endfor

clean:
.for i in $(SUBDIRS)
	@echo "==> Enter subdirectory $(i)"
	@cd ${i}; make $@
	@echo "<== Leave subdirectory $(i)"
.endfor
