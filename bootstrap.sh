# This script will download the FFTW library,
# compile it, and then install it to a folder
# called "local".

WOscLib_URL=http://easynews.dl.sourceforge.net/sourceforge/wosclib/WOscLib-00.05.zip
WOscLib_TAR=WOscLib-00.05.zip
WOscLib_DIR=WOscLib
WOscLib_MD5=2218191f2f3f335ae80b4ebd616d77ac

if [ $(md5sum $WOscLib_TAR | sed "s,.*= *\(.*\),\1,g")x != ${WOscLib_MD5}x ]; then
	echo Downloading $WOscLib_TAR ...
	rm -v $WOscLib_TAR
	wget -O $WOscLib_TAR $WOscLib_URL
fi

if [ $(md5sum $WOscLib_TAR | sed "s,.*= *\(.*\),\1,g")x != ${WOscLib_MD5}x ]; then
	echo "Error in MD5 checksum for $WOscLib_TAR"
	exit
fi

echo Extracting $WOscLib_TAR ...
if !(unzip -o $WOscLib_TAR); then
	echo "Error in archive.";
fi

cd $WOscLib_DIR

# The rest of this script is only for Linux.
if [ "$(uname)"x != "Linux"x ]; then
    echo Done.
    echo Please compile $WOscLib_DIR manually.
    exit
fi

echo Patching $WOscLib_DIR
if !(patch -p1 <<EOF
diff -ru WOscLib/WOscContainer.h WOscLib-new/WOscContainer.h
--- WOscLib/WOscContainer.h	2006-05-16 23:21:42.000000000 -0400
+++ WOscLib-new/WOscContainer.h	2006-12-20 12:01:09.000000000 -0500
@@ -64,6 +64,7 @@
 /*  -----------------------------------------------------------------------  */
 
 class WOscCallbackList;
+class WOscMethod;
 
 /** OSC Address-space node.
  * An OSC-address space consists of a tree of containers with methods as leaves.
EOF
); then
	echo "Error applying Linux patch."
	exit
fi

echo Compiling $WOscLib_DIR ...
if !(cd build/linux && make -f makefile-linux-a.mk); then
	echo "Error compiling $WOscLib_DIR."
	exit
fi

echo Done.
