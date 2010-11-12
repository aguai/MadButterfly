#!/bin/sh

ABS=`realpath $0`
BASE=`dirname $ABS`
NODE_PATH=$BASE:$NODE_PATH
export NODE_PATH

DIR=`dirname $1`
FNAME=`basename $1`

cd $DIR; node $FNAME
