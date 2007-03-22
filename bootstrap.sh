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

if [ $($MD5 $liblo_TAR | $MD5CUT)x != ${liblo_MD5}x ]; then
	echo Downloading $liblo_TAR ...
	rm -v $liblo_TAR
    $DL $liblo_TAR $liblo_URL
fi

if [ $($MD5 $liblo_TAR | $MD5CUT)x != ${liblo_MD5}x ]; then
	echo "Error in MD5 checksum for $liblo_TAR"
	exit
fi

if ! [ -d $liblo_DIR ]; then
echo Extracting $liblo_TAR ...
if !(tar -xzf $liblo_TAR); then
	echo "Error in archive.";
	exit
fi

if [ ${liblo_PATCH}x != x ]; then
echo Patching $liblo_DIR
if !(cd $liblo_DIR && patch -p1 <../$liblo_PATCH); then
	echo "Error applying patch" $liblo_PATCH
	exit
fi
fi

case $(uname) in
   CYGWIN*)
   ;;
   *)
echo Configuring $liblo_DIR
if !(cd $liblo_DIR && ./configure --disable-shared); then
	echo "Error configuring $liblo_DIR"
	exit
fi

echo Compiling $liblo_DIR
if !(cd $liblo_DIR && make); then
	echo "Error compiling $liblo_DIR"
	exit
fi
    ;;
esac
fi

echo
echo LibLo Done.
echo
case $(uname) in
   CYGWIN*)
echo Now open solution file "$(cygpath -w $liblo_DIR/LibLo.sln)" in \
    Visual Studio 2003 and Build All.
explorer "$(cygpath -w $liblo_DIR)"
   ;;
esac
}

ode() {
ode_URL=http://internap.dl.sourceforge.net/sourceforge/opende/ode-src-0.7.zip
ode_TAR=ode-src-0.7.zip
ode_DIR=ode-0.7
ode_MD5=b6727fef2cbb9ca812438bb774c9d6ec

if [ $($MD5 $ode_TAR | $MD5CUT)x != ${ode_MD5}x ]; then
	echo Downloading $ode_TAR ...
	rm -v $ode_TAR
	$DL $ode_TAR $ode_URL
fi

if [ $($MD5 $ode_TAR | $MD5CUT)x != ${ode_MD5}x ]; then
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
rm -v $ode_DIR/ode/src/libode.so

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

chai3d() {
chai_URL=http://chai3d.org/builds/chai3d%5Bv1.51%5D.zip
chai_TAR=chai3d\[v1.51\].zip
chai_MD5=006c5464dc3c389d87e615c24ccd9696

if [ $($MD5 "$chai_TAR" | $MD5CUT)x != ${chai_MD5}x ]; then
    echo Downloading $chai_TAR ...
    rm -v $chai_TAR
    $DL "$chai_TAR" $chai_URL
fi

if [ $($MD5 "$chai_TAR" | $MD5CUT)x != ${chai_MD5}x ]; then
    echo "Error in MD5 checksum for $chai_TAR"
    exit
fi

if ! [ -d $chai_DIR ]; then
echo Extracting "$chai_TAR" ...
if !(unzip -o "$chai_TAR"); then
    echo "Error in archive.";
    exit
fi

if [ ${chai_PATCH}x != x ]; then
echo Patching $chai_DIR
if !(patch -p0 <$chai_PATCH); then
	echo "Error applying patch" $chai_PATCH
	exit
fi
fi

echo Compiling $chai_DIR
if !(cd $chai_DIR && make); then
    echo "Error compiling $chai_DIR"
    exit
fi
fi

echo
echo Chai3d Done.
echo
}

# System-dependant bootstrapping
case $(uname) in
    CYGWIN*)
	DL="wget -O"
    MD5=md5sum
	MD5CUT="awk {print\$1}"
    liblo_PATCH=liblo-0.23-msvc7.patch
    ode_PATCH=ode-0.23-msvc7.patch
#    ode
    liblo
    ;;

    Linux*)
	DL="wget -O"
    MD5=md5sum
	MD5CUT="awk {print\$1}"
	chai_DIR=chai3d/linux

	ode
    chai3d
    echo For Linux, please get package \"liblo\" from your distribution.
    echo Ubuntu and Debian: sudo apt-get install liblo0-dev
    exit
	;;

	Darwin*)
	DL="curl -o"
    MD5=md5
	MD5CUT="cut -f2 -d="
    chai_PATCH=chai3d-1.51-darwin.patch
	chai_DIR=chai3d/darwin
    ode
	liblo
	chai3d
    ;;
    *)
    echo Your system is not supported by this script.
    echo Please acquire the \"liblo\" and \"ode\" packages manually.
    exit
    ;;
esac

