#!/bin/sh

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

olddir=`pwd`
cd $srcdir

srcdir=`php -r "echo realpath('$srcdir');"`

echo "Working Dir: $srcdir"

$srcdir/configure --enable-shared --prefix=$srcdir/bin || {
	echo "error: configure failed"
	exit 1
}

make || {
	echo "error: make failed"
	exit 1
}

make install || {
	echo "error: make install failed"
	exit 1
}
