#!/bin/bash

echo git fetch --unshallow
git fetch --unshallow
echo util/version.sh
util/version.sh

git clone http://github.com/radarsat1/dimple.git pages
cd pages
git checkout gh-pages
cd ..

DIMPLE=dimple-$TRAVIS_OS_NAME-`util/version.sh`
if [ -e install/bin/dimple ]; then
  cp -rv install/bin/dimple pages/$DIMPLE
elif [ -e install/bin/dimple.exe ]; then
  DIMPLE=dimple-mingw-`util/version.sh`.exe
  cp -rv install/bin/dimple.exe pages/$DIMPLE
else
  echo "No installed dimple executable found:"
  find install
  exit 1
fi
echo '<p><a href="'$DIMPLE'">'$DIMPLE'</a>' >>pages/index.html

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
  if [ -e install/bin/dimple ]; then
    ldd install/bin/dimple
  elif [ -e install/bin/dimple.exe ]; then
    echo -n
  fi
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
