extdir=$(datadir)/inkscape/extenstions
ext_py=AssignSymbol.py MB_EditButton.py MB_Frame.py
ext_DATA=$(ext_py) MB_EditButton.inx MB_Frame.inx MB_assignSymbol.inx mbtest.svg

install-data-hook:
	for file in $(ext_py); do cd "$(DESTDIR)$(extdir)"; tgt=$${file#inkscape/}; chmod +x $${tgt}; done

EXTRA_DIST=$(ext_DATA)
