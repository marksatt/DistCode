#!/bin/sh

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

olddir=`pwd`
cd $srcdir

srcdir=`php -r "echo realpath('$srcdir');"`
echo "Working Dir: $srcdir"

export PATH=$PATH:/usr/local/bin
export LIBRARY_PATH=$LIBRARY_PATH:$srcdir/../lzo-2.08/bin/lib
export C_INCLUDE_PATH=$C_INCLUDE_PATH:$srcdir/../lzo-2.08/bin/include
export CPATH=$CPATH:$srcdir/../lzo-2.08/bin/include
export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:$srcdir/../lzo-2.08/bin/include
export OBJC_INCLUDE_PATH=$OBJC_INCLUDE_PATH:$srcdir/../lzo-2.08/bin/include

[ -f $srcdir/configure ] || {
	sh $srcdir/autogen.sh || {
		echo "error: autogen failed"
		exit 1
	}

	$srcdir/configure --without-man --prefix=$srcdir/bin || {
		echo "error: configure failed"
		exit 1
	}
}

make || {
	echo "error: make failed"
	exit 1
}

make install || {
	echo "error: make install failed"
	exit 1
}

rm $srcdir/bin/lib/liblzo2.2.dylib

cp $srcdir/../lzo-2.08/bin/lib/liblzo2.2.dylib $srcdir/bin/lib || {
	echo "error: cp liblzo2.dylib failed"
	exit 1
}

dylib=`php -r "echo realpath('$srcdir/../lzo-2.08/bin/lib/liblzo2.2.dylib');"`
echo "Dylib: $dylib"

xcrun install_name_tool -change $dylib @loader_path/../lib/liblzo2.2.dylib $srcdir/bin/bin/icecc || {
	echo "error: install_name_tool icecc failed"
	exit 1
}

xcrun install_name_tool -change $dylib @loader_path/../lib/liblzo2.2.dylib $srcdir/bin/sbin/iceccd || {
	echo "error: install_name_tool iceccd failed"
	exit 1
}

xcrun install_name_tool -change $dylib @loader_path/../lib/liblzo2.2.dylib $srcdir/bin/sbin/icecc-scheduler || {
	echo "error: install_name_tool icecc-scheduler failed"
	exit 1
}
