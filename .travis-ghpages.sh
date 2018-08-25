#!/bin/bash

echo find install
find install
echo find pages
find pages

git clone http://github.com/radarsat1/dimple.git pages
cd pages
git checkout gh-pages
cd ..

DIMPLE=dimple-$TRAVIS_OS_NAME-`git describe --always`
cp -rv install/bin/dimple pages/$DIMPLE
echo '<p><a href="$DIMPLE">'$DIMPLE'</a>' >>pages/index.html

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    ldd install/bin/dimple
elif [ "$TRAVIS_OS_NAME" = "osx" ]; then
    otool -L pages/$DIMPLE
    ls -l /usr/local/opt/libusb/lib/
    cp -v /usr/local/opt/libusb/lib/libusb-1.0.0.dylib pages/
    install_name_tool -change /usr/local/opt/libusb/lib/libusb-1.0.0.dylib "@loader_path/libusb-1.0.0.dylib" pages/$DIMPLE
    otool -L pages/$DIMPLE
    if ! grep -q libusb-1.0.0.dylib pages/index.html; then
        echo '<p><a href="libusb-1.0.0.dylib">'libusb-1.0.0.dylib'</a>' >>pages/index.html
    fi
fi
