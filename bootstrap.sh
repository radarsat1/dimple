# This script will download the LibLo library,
# and patch it so it can be compiled using
# Visual Studio 2003 (MSVC7).

# For Windows, this script is intended to be run using Cygwin.
# Required packages: tar, patch, coreutils

# Check that we are using Cygwin.
echo This script bootstraps required libraries for selected environments.
case $(uname) in
    CYGWIN*)
    ;;
    Linux*)
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

echo Done.
echo Now open solution file "$(cygpath -w $liblo_DIR/LibLo.sln)" in \
    Visual Studio 2003 and Build All.
explorer "$(cygpath -w $liblo_DIR)"
