#!/bin/sh

autoreconf -ifs
./configure --host mipsel-linux --build i686-pc-linux-gnu --prefix ${TANGO} --disable-sh_text --with-backend=dfb
make && make install
