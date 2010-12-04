#!/bin/sh

ABS=`realpath $0`
if [ -z "$SRCDIR" ]; then
    SRCDIR=`dirname $ABS`/..
fi
if [ -z "$BUILDDIR" ]; then
    BUILDDIR=`dirname $ABS`/..
fi

NODE_PATH=${BUILDDIR}/nodejs/objs/default:${SRCDIR}/nodejs:$NODE_PATH
export NODE_PATH

LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${BUILDDIR}/src/.libs/
export LD_LIBRARY_PATH

DIR=`dirname $1`
FNAME=`basename $1`

cd $DIR; node $FNAME
