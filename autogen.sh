#!/bin/sh

aclocal
autoheader
libtoolize --automake --copy
automake --add-missing --foreign
autoconf
rm -fr autom4te.cache
