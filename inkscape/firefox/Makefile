all: madbuilder.xpi inkscape-mb

madbuilder.xpi: chrome.manifest install.rdf content/*
	zip madbuilder.xpi chrome.manifest  content/* install.rdf README

inkscape-0.46.tar.gz:
	wget -O inkscape-0.46.tar.gz http://nchc.dl.sourceforge.net/sourceforge/inkscape/inkscape-0.46.tar.gz

inkscape-0.46/.configured: inkscape-0.46.tar.gz
	tar xzvf inkscape-0.46.tar.gz
	cd inkscape-0.46; patch -p1 < ../inkscape-mb-patch.diff ; ./configure
	touch inkscape-0.46/.configured

inkscape-0.46/.compiled: inkscape-0.46/.configured
	cd inkscape-0.46; make -j 2
	touch inkscape-0.46/.compiled

inkscape-mb: inkscape-0.46/.compiled
	rm -rf inkscape-0.46/dest
	- mkdir -p inkscape-0.46/dest/usr/local
	cd inkscape-0.46; make prefix=`pwd`/dest/usr/local install
	cp MBFilter.inx MBServer.py testsoap.py inkscape-0.46/dest/usr/local/share/inkscape/extensions
	cp helper_mb.py  inkscape-0.46/dest/usr/local/bin
	chmod +x inkscape-0.46/dest/usr/local/bin/helper_mb.py
	mv inkscape-0.46/dest/usr/local/bin/inkscape inkscape-0.46/dest/usr/local/bin/inkscape-mb
	cd inkscape-0.46/dest; tar czvf ../../inkscape-mb.tgz usr



mozplugger:


clean:
	rm -f inkscape-0.46/.configured inkscape-0.46/.compiled
	make -C inkscape-0.46 clean
