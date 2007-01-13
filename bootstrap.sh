# This script will download the LibLo library,
# and patch it so it can be compiled using
# Visual Studio 2003 (MSVC7).

# For Windows, this script is intended to be run using Cygwin.
# Required packages: tar, patch, coreutils

echo This script bootstraps required libraries for selected environments.

liblo() {
liblo_URL=http://easynews.dl.sourceforge.net/sourceforge/liblo/liblo-0.23.tar.gz
liblo_TAR=liblo-0.23.tar.gz
liblo_DIR=liblo-0.23
liblo_MD5=e14c9f4fae7ed8d9622d126f6fb9c1d7
liblo_PATCH=liblo-0.23-msvc7.patch

if [ $(md5sum $liblo_TAR | cut -d" " -f1)x != ${liblo_MD5}x ]; then
	echo Downloading $liblo_TAR ...
	rm -v $liblo_TAR
	wget -O $liblo_TAR $liblo_URL
fi

if [ $(md5sum $liblo_TAR | cut -d" " -f1)x != ${liblo_MD5}x ]; then
	echo "Error in MD5 checksum for $liblo_TAR"
	exit
fi

if ! [ -d $liblo_DIR]; then
echo Extracting $liblo_TAR ...
if !(tar -xzf $liblo_TAR); then
	echo "Error in archive.";
	exit
fi

echo Patching $liblo_DIR
if !(cd $liblo_DIR && patch -p1 <../$liblo_PATCH); then
	echo "Error applying MSVC7 patch."
	exit
fi
fi

echo
echo LibLo Done.
echo
echo Now open solution file "$(cygpath -w $liblo_DIR/LibLo.sln)" in \
    Visual Studio 2003 and Build All.
explorer "$(cygpath -w $liblo_DIR)"
}

ode() {
ode_URL=http://internap.dl.sourceforge.net/sourceforge/opende/ode-src-0.7.zip
ode_TAR=ode-src-0.7.zip
ode_DIR=ode-0.7
ode_MD5=b6727fef2cbb9ca812438bb774c9d6ec
#ode_PATCH=ode-0.23-msvc7.patch

if [ $(md5sum $ode_TAR | cut -d" " -f1)x != ${ode_MD5}x ]; then
	echo Downloading $ode_TAR ...
	rm -v $ode_TAR
	wget -O $ode_TAR $ode_URL
fi

if [ $(md5sum $ode_TAR | cut -d" " -f1)x != ${ode_MD5}x ]; then
	echo "Error in MD5 checksum for $ode_TAR"
	exit
fi

if ! [ -d $ode_DIR ]; then
echo Extracting $ode_TAR ...
if !(unzip -o $ode_TAR); then
	echo "Error in archive.";
	exit
fi

#echo Patching $ode_DIR
#if !(cd $ode_DIR && patch -p1 <../$ode_PATCH); then
#	echo "Error applying MSVC7 patch."
#	exit
#fi

echo Configuring $ode_DIR
if !(cd $ode_DIR && ./configure --disable-shared); then
	echo "Error configuring $ode_DIR"
	exit
fi

# Seems to make the shared version anyway.. ?
rm $ode_DIR/ode/src/libode.so

echo Compiling $ode_DIR
if !(cd $ode_DIR && make); then
	echo "Error compiling $ode_DIR"
	exit
fi
fi

echo
echo ODE Done.
echo
}


# System-dependant bootstrapping
case $(uname) in
    CYGWIN*)
#    ode
    liblo
    ;;
    Linux*)
	ode
    echo For Linux, please get package \"liblo\" from your distribution.
    echo Ubuntu and Debian: sudo apt-get install liblo0-dev
    exit
    ;;
    *)
    echo Your system is not supported by this script.
    echo Please acquire the \"liblo\" package manually.
    exit
    ;;
esac



