#!/bin/bash

echo find install
find install
echo find pages
find pages

git clone http://github.com/radarsat1/dimple.git pages
cd pages
git checkout gh-pages
cd ..

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    ldd install/bin/dimple
elif [ "$TRAVIS_OS_NAME" = "osx" ]; then
    otool -L install/bin/dimple
    ls -l /usr/local/opt/libusb/lib/
    cp -v /usr/local/opt/libusb/lib/libusb-1.0.0.dylib pages/)
fi

cp -rv install/bin/dimple pages/dimple-`uname -s`
echo dimple-`uname -s` >>pages/index.html
