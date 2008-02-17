# This script will download the LibLo library,
# and patch it so it can be compiled using
# Visual Studio 2003 (MSVC7).

# For Windows, this script is intended to be run using Cygwin.
# Required packages: tar, patch, coreutils

echo This script bootstraps required libraries for selected environments.

liblo() {
liblo_URL=http://easynews.dl.sourceforge.net/sourceforge/liblo/liblo-0.24.tar.gz
liblo_TAR=liblo-0.24.tar.gz
liblo_DIR=liblo-0.24
liblo_MD5=a9b5e7c6fcc835cd468e26cc95aba91a

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
   echo Compiling $liblo_DIR Debug
   if !( "$COMPILE" /Build Debug $(cygpath -w $liblo_DIR/LibLo.sln ) /Project LibLo /Out compile.log ); then
	  echo "Error compiling $liblo_DIR" Debug
	  cat compile.log
	  exit
   fi
   rm compile.log >/dev/null 2>&1
   echo Compiling $liblo_DIR Release
   if !( "$COMPILE" /Build Release $(cygpath -w $liblo_DIR/LibLo.sln ) /Project LibLo /Out compile.log ); then
	  echo "Error compiling $liblo_DIR" Release
	  cat compile.log
	  exit
   fi
   rm compile.log >/dev/null 2>&1
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

if [ ${ode_PATCH}x != x ] && [ -f $ode_PATCH ]; then
echo Patching $ode_DIR
if !(cd $ode_DIR && patch -p1 <../$ode_PATCH); then
	echo "Error applying patch " $ode_PATCH
	exit
fi
fi

case $(uname) in
	CYGWIN*)
    echo Compiling $ode_DIR DebugLib
    if !( "$COMPILE" /Build DebugLib $(cygpath -w $ode_DIR/$ode_SLN ) /Project ode /Out compile.log ); then
	   echo "Error compiling $ode_DIR" DebugLib
	   cat compile.log
	   exit
    fi
    rm compile.log >/dev/null 2>&1
    echo Compiling $ode_DIR ReleaseLib
    if !( "$COMPILE" /Build ReleaseLib $(cygpath -w $ode_DIR/$ode_SLN ) /Project ode /Out compile.log ); then
	   echo "Error compiling $ode_DIR" ReleaseLib
	   cat compile.log
	   exit
    fi
    rm compile.log >/dev/null 2>&1
	;;

	*)
    echo Configuring $ode_DIR
    if !(cd $ode_DIR && ./configure --disable-shared); then
    	echo "Error configuring $ode_DIR"
	    exit
    fi

    echo Compiling $ode_DIR
    if !(cd $ode_DIR && make); then
	    echo "Error compiling $ode_DIR"
    	exit
    fi

    # Seems to make the shared version anyway.. ?
    rm -v $ode_DIR/ode/src/libode.so $ode_DIR/ode/src/libode.dylib
    ;;
esac

fi

echo
echo ODE Done.
echo
}

chai3d() {
chai_URL=http://chai3d.org/builds/chai3d%5Bv1.61%5D.zip
chai_TAR=chai3d-v1.61.zip
chai_MD5=516eebf36ca995b9f200b965cc78f002

if ! [ -d $chai_DIR ]; then

if [ $($MD5 "$chai_TAR" | $MD5CUT)x != ${chai_MD5}x ]; then
    echo Downloading $chai_TAR ...
    rm -v $chai_TAR
    $DL "$chai_TAR" $chai_URL
fi

if [ $($MD5 "$chai_TAR" | $MD5CUT)x != ${chai_MD5}x ]; then
    echo "Error in MD5 checksum for $chai_TAR"
    exit
fi
fi

if ! [ -d $chai_DIR ]; then
echo Extracting "$chai_TAR" ...
if !(unzip -o "$chai_TAR"); then
    echo "Error in archive.";
    exit
fi

# TODO: change this to a -p1 patch to avoid confusion!
if [ ${chai_PATCH}x != x ]; then
echo Patching chai3d
if !(cd chai3d; patch -p1 <../$chai_PATCH); then
	echo "Error applying patch" $chai_PATCH
	exit
fi
fi

case $(uname) in
	CYGWIN*)
    echo Compiling $chai_DIR Debug
    if !( "$COMPILE" /Build Debug $(cygpath -w $chai_DIR/chai3d_complete.sln ) /Project chai3d_complete /Out compile.log ); then
       echo "Error compiling $chai_DIR" Debug
       cat compile.log
  	   exit
    fi
    rm compile.log >/dev/null 2>&1
    echo Compiling $chai_DIR Release
    if !( "$COMPILE" /Build Release $(cygpath -w $chai_DIR/chai3d_complete.sln ) /Project chai3d_complete /Out compile.log ); then
       echo "Error compiling $chai_DIR" Release
       cat compile.log
  	   exit
    fi
    rm compile.log >/dev/null 2>&1
	;;

	*)
    echo Compiling $chai_DIR
    if !(cd $chai_DIR && make); then
        echo "Error compiling $chai_DIR"
        exit
    fi
    ;;
esac

fi

echo
echo Chai3d Done.
echo
}

freeglut() {
freeglut_URL=http://internap.dl.sourceforge.net/sourceforge/freeglut/freeglut-2.4.0.tar.gz
freeglut_TAR=freeglut-2.4.0.tar.gz
freeglut_DIR=freeglut-2.4.0
freeglut_MD5=6d16873bd876fbf4980a927cfbc496a1

if ! [ -d $freeglut_DIR ]; then

if [ $($MD5 "$freeglut_TAR" | $MD5CUT)x != ${freeglut_MD5}x ]; then
    echo Downloading $freeglut_TAR ...
    rm -v $freeglut_TAR
    $DL "$freeglut_TAR" $freeglut_URL
fi

if [ $($MD5 "$freeglut_TAR" | $MD5CUT)x != ${freeglut_MD5}x ]; then
    echo "Error in MD5 checksum for $freeglut_TAR"
    exit
fi
fi

if ! [ -d $freeglut_DIR ]; then
echo Extracting "$freeglut_TAR" ...
if !(tar -xzf "$freeglut_TAR"); then
    echo "Error in archive.";
    exit
fi

if [ ${freeglut_PATCH}x != x ]; then
echo Patching $freeglut_DIR
if !(cd $freeglut_DIR && patch -p1 <../$freeglut_PATCH); then
	echo "Error applying patch" $freeglut_PATCH
	exit
fi
fi

case $(uname) in
	CYGWIN*)
	if [ ${vs_VER}x == 2003x ]; then
		echo Setting back version for solution \& project
		sed 's/Version [0-9,.]*/Version 8.00/' $freeglut_DIR/freeglut.sln --in-place
		sed 's/Version="[0-9,.]*"/Version="7.10"/' $freeglut_DIR/freeglut.vcproj --in-place
		sed 's/Version="[0-9,.]*"/Version="7.10"/' $freeglut_DIR/freeglut_static.vcproj --in-place
	fi
    echo Compiling $freeglut_DIR Debug
    if !( "$COMPILE" /Build Debug $(cygpath -w $freeglut_DIR/freeglut.sln ) /Project freeglut_static /Out compile.log ); then
       echo "Error compiling $freeglut_DIR" Debug
       cat compile.log
  	   exit
    fi
    rm compile.log >/dev/null 2>&1
    echo Compiling $freeglut_DIR Release
    if !( "$COMPILE" /Build Release $(cygpath -w $freeglut_DIR/freeglut.sln ) /Project freeglut_static /Out compile.log ); then
       echo "Error compiling $freeglut_DIR" Release
       cat compile.log
  	   exit
    fi
    rm compile.log >/dev/null 2>&1
	;;

    *)
    echo Configuring $freeglut_DIR
    if !(cd $freeglut_DIR && env CFLAGS=-DFREEGLUT_STATIC ./configure --prefix=/mingw --disable-shared); then
        echo "Error configuring $freeglut_DIR"
        exit
    fi

    echo Compiling $freeglut_DIR
    if !(cd $freeglut_DIR && make); then
	    echo "Error compiling $freeglut_DIR"
    	exit
    fi    
    ;;
esac

fi

echo
echo FreeGLUT Done.
echo
}

atomicops() {
atomicops_URL=http://www.hpl.hp.com/research/linux/atomic_ops/download/libatomic_ops-1.2.tar.gz
atomicops_TAR=libatomic_ops-1.2.tar.gz
atomicops_DIR=libatomic_ops-1.2
atomicops_MD5=1b65e48271c81e3fa2d7a9a69bab7504

if ! [ -d $atomicops_DIR ]; then

if [ $($MD5 "$atomicops_TAR" | $MD5CUT)x != ${atomicops_MD5}x ]; then
    echo Downloading $atomicops_TAR ...
    rm -v $atomicops_TAR
    $DL "$atomicops_TAR" $atomicops_URL
fi

if [ $($MD5 "$atomicops_TAR" | $MD5CUT)x != ${atomicops_MD5}x ]; then
    echo "Error in MD5 checksum for $atomicops_TAR"
    exit
fi
fi

if ! [ -d $atomicops_DIR ]; then
echo Extracting "$atomicops_TAR" ...
if !(tar -xzf "$atomicops_TAR"); then
    echo "Error in archive.";
    exit
fi

if [ ${atomicops_PATCH}x != x ]; then
echo Patching $atomicops_DIR
if !(cd $atomicops_DIR && patch -p1 <../$atomicops_PATCH); then
	echo "Error applying patch" $atomicops_PATCH
	exit
fi
fi

fi

echo
echo atomic_ops Done.
echo
}


pthreads() {
pthreads_URL=ftp://sourceware.org/pub/pthreads-win32/pthreads-w32-2-8-0-release.tar.gz
pthreads_TAR=pthreads-w32-2-8-0-release.tar.gz
pthreads_DIR=pthreads-w32-2-8-0-release
pthreads_MD5=6d30c693233b1464ef8983fedd8ccb22

if ! [ -d $pthreads_DIR ]; then

if [ $($MD5 "$pthreads_TAR" | $MD5CUT)x != ${pthreads_MD5}x ]; then
    echo Downloading $pthreads_TAR ...
    rm -v $pthreads_TAR
    $DL "$pthreads_TAR" $pthreads_URL
fi

if [ $($MD5 "$pthreads_TAR" | $MD5CUT)x != ${pthreads_MD5}x ]; then
    echo "Error in MD5 checksum for $pthreads_TAR"
    exit
fi
fi

if ! [ -d $pthreads_DIR ]; then
echo Extracting "$pthreads_TAR" ...
if !(tar -xzf "$pthreads_TAR"); then
    echo "Error in archive.";
    exit
fi

if [ ${pthreads_PATCH}x != x ]; then
echo Patching $pthreads_DIR
if !(cd $pthreads_DIR && patch -p1 <../$pthreads_PATCH); then
	echo "Error applying patch" $pthreads_PATCH
	exit
fi
fi

case $(uname) in
	CYGWIN*)
	if [ ${vs_VER}x == 2003x ]; then
		echo Setting back version for solution \& project
		sed 's/Version [0-9,.]*/Version 8.00/' $pthreads_DIR/pthreads.sln --in-place
		sed 's/Version="[0-9,.]*"/Version="7.10"/' $pthreads_DIR/pthreads.vcproj --in-place
	fi

    echo Compiling $pthreads_DIR Debug
    if !( "$COMPILE" /Build Debug $(cygpath -w $pthreads_DIR/pthreads.sln ) /Project pthreads /Out compile.log ); then
       echo "Error compiling $pthreads_DIR" Debug
       cat compile.log
  	   exit
    fi
    rm compile.log >/dev/null 2>&1
    echo Compiling $pthreads_DIR Release
    if !( "$COMPILE" /Build Release $(cygpath -w $pthreads_DIR/pthreads.sln ) /Project pthreads /Out compile.log ); then
       echo "Error compiling $pthreads_DIR" Release
       cat compile.log
  	   exit
    fi
    rm compile.log >/dev/null 2>&1
	;;
esac

fi

echo
echo pthreads Done.
echo
}

scons() {
scons_URL=http://superb-west.dl.sourceforge.net/sourceforge/scons/scons-local-0.96.1.tar.gz
scons_TAR=scons-local-0.96.1.tar.gz
scons_MD5=78754efc02b4a374d5082a61509879cd
scons_DIR=scons-local-0.96.1

if ! [ -d $scons_DIR ]; then

if [ $($MD5 "$scons_TAR" | $MD5CUT)x != ${scons_MD5}x ]; then
    echo Downloading $scons_TAR ...
    rm -v $scons_TAR
    $DL "$scons_TAR" $scons_URL
fi

if [ $($MD5 "$scons_TAR" | $MD5CUT)x != ${scons_MD5}x ]; then
    echo "Error in MD5 checksum for $scons_TAR"
    exit
fi
fi

if ! [ -d $scons_DIR ]; then
echo Extracting "$scons_TAR" ...
if !(tar -xzf "$scons_TAR"); then
    echo "Error in archive.";
    exit
fi
fi

}

samplerate() {
samplerate_URL="http://www.mega-nerd.com/SRC/libsamplerate-0.1.2.tar.gz"
samplerate_TAR=libsamplerate-0.1.2.tar.gz
samplerate_DIR=libsamplerate-0.1.2
samplerate_MD5=06861c2c6b8e5273c9b80cf736b9fd0e

if ! [ -d $samplerate_DIR ]; then

if [ $($MD5 "$samplerate_TAR" | $MD5CUT)x != ${samplerate_MD5}x ]; then
    echo Downloading $samplerate_TAR ...
    rm -v $samplerate_TAR
    $DL "$samplerate_TAR" $samplerate_URL
fi

if [ $($MD5 "$samplerate_TAR" | $MD5CUT)x != ${samplerate_MD5}x ]; then
    echo "Error in MD5 checksum for $samplerate_TAR"
    exit
fi
fi

if ! [ -d $samplerate_DIR ]; then
echo Extracting "$samplerate_TAR" ...
if !(tar -xzf "$samplerate_TAR"); then
    echo "Error in archive.";
    exit
fi

if [ ${samplerate_PATCH}x != x ]; then
echo Patching $samplerate_DIR
if !(cd $samplerate_DIR && patch -p1 <../$samplerate_PATCH); then
	echo "Error applying patch" $samplerate_PATCH
	exit
fi
fi

case $(uname) in
	CYGWIN*)
    echo Compiling $samplerate_DIR Debug
    if !( "$COMPILE" $(cygpath -w $samplerate_DIR/Win32/libsamplerate-vs2005exp.sln ) /Build Debug /Project libsamplerate-vs2005exp /Out compile.log ); then
       echo "Error compiling $samplerate_DIR" Debug
       cat compile.log
  	   exit
    fi
    rm compile.log >/dev/null 2>&1
    echo Compiling $samplerate_DIR Release
    if !( "$COMPILE" /Build Release $(cygpath -w $samplerate_DIR/Win32/libsamplerate-vs2005exp.sln ) /Project libsamplerate-vs2005exp /Out compile.log ); then
       echo "Error compiling $samplerate_DIR" Release
       cat compile.log
  	   exit
    fi
    rm compile.log >/dev/null 2>&1
	;;

   *)
   echo Configuring $samplerate_DIR
   if !(cd $samplerate_DIR && ./configure --disable-shared); then
	  echo "Error configuring $samplerate_DIR"
	  exit
   fi

   echo Compiling $samplerate_DIR
   if !(cd $samplerate_DIR && make); then
	  echo "Error compiling $samplerate_DIR"
	  exit
   fi
   ;;
esac

fi

echo
echo samplerate Done.
echo
}

cd libdeps

# System-dependant bootstrapping
case $(uname) in
    MINGW32*)
    DL="wget -O"
    MD5=md5sum
	MD5CUT="awk '{print\$1}'"
    freeglut_PATCH=freeglut-2.4.0-mingw.patch

    freeglut
    ;;

    CYGWIN*)
	DL="wget -O"
    MD5=md5sum
	MD5CUT="awk {print\$1}"
	freeglut_PATCH=freeglut-2.4.0-vs2005exp.patch
	pthreads_PATCH=pthreads-w32-2-8-0-release-vs2005exp-static.patch
	chai_PATCH=chai3d-1.61-vs2005exp.patch

	COMPILE="$(echo $(cygpath -u $PROGRAMFILES)/Microsoft Visual Studio .NET 2003/Common7/IDE/devenv.exe)"
	if !( [ -f "$COMPILE" ]); then
		COMPILE="$(echo $(cygpath -u $PROGRAMFILES)/Microsoft Visual Studio 8/Common7/IDE/VCExpress.exe)"
		if !( [ -f "$COMPILE" ]); then
			echo "Couldn't find Visual Studio 2003 or 2005 Express.  Please edit the line COMPILE= in this file (bootstrap.sh)"
			exit
		else
			vs_VER=2005
			ode_SLN=build/vs2005/ode.sln
			ode_PATCH=ode-0.7-vs2005exp.patch
			liblo_PATCH=liblo-0.23-vs2005exp.patch
			samplerate_PATCH=libsamplerate-0.1.2-vs2005exp.patch
			chai_DIR=chai3d/msvc8exp
		fi
	else
		vs_VER=2003
		ode_SLN=build/vs2003/ode.sln
		ode_PATCH=ode-0.7-msvc7.patch
		liblo_PATCH=liblo-0.23-msvc7.patch
		chai_DIR=chai3d/msvc7
	fi

	pthreads
	freeglut
    samplerate
    ode
    liblo
	chai3d
	atomicops

	cd ..
	if [ ${vs_VER}x == 2003x ]; then
		echo Setting back version of dimple solution \& project for Visual Studio 2003
		sed 's/Version [0-9,.]*/Version 8.00/' dimple.sln --in-place
		sed 's/Version="[0-9,.]*"/Version="7.10"/' src/dimple.vcproj --in-place
		sed 's,chai3d/lib/msvc8exp,chai3d/lib/msvc7,' src/dimple.vcproj --in-place
	fi

	echo Now open dimple.sln in Visual Studio and build.
    ;;

    Linux*)
	DL="wget -O"
    MD5=md5sum
	MD5CUT="awk {print\$1}"
	chai_DIR=chai3d/linux

    scons
	ode
    chai3d
	liblo
#    echo For Linux, please get package \"liblo\" from your distribution.
#    echo Ubuntu and Debian: sudo apt-get install liblo0-dev
    exit
	;;

	Darwin*)
	DL="curl -o"
    MD5=md5
	MD5CUT="cut -f2 -d="
    chai_PATCH=chai3d-1.61-darwin.patch
	chai_DIR=chai3d/darwin
    scons
    samplerate
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

