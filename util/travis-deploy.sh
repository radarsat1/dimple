#!/bin/bash

set -e
set -x

# HUGO=/snap/hugo/current/bin/hugo
HUGO=hugo

# git fetch --unshallow
VERSION=$(util/version.sh)

DIMPLE=dimple-$TRAVIS_OS_NAME-${VERSION}
case $TRAVIS_OS_NAME in
    osx) OS=mac; util/build-appbundle.sh;;
    linux) OS=linux; [ -z "$MINGW_ON_LINUX" ] && util/build-appimage.sh || util/build-nsis.sh;;
esac

if [ -e nsis/dimple-${VERSION}-win64-installer.exe ]; then
  DIMPLE=dimple-${VERSION}-win64-installer.exe
  OS=windows
  cp -rv nsis/$DIMPLE ./
elif [ -e dimple-${VERSION}-x86_64.AppImage ]; then
  DIMPLE=dimple-${VERSION}-x86_64.AppImage
elif [ -e dimple-${VERSION}-mac-x86_64.dmg ]; then
  DIMPLE=dimple-${VERSION}-mac-x86_64.dmg
elif [ -e inst/bin/dimple.exe ]; then
  DIMPLE=dimple-mingw-${VERSION}.exe
  OS=windows
  cp -rv inst/bin/dimple.exe $DIMPLE
elif [ -e inst/bin/dimple ]; then
  DIMPLE=dimple-mingw-${VERSION}
  cp -rv inst/bin/dimple $DIMPLE
else
  echo "No installed dimple executable found:"
  find inst
  exit 1
fi

# Release artifact
export FILE_TO_UPLOAD="$DIMPLE"

## Hugo site (if linux build)
if [ $OS = linux ]; then

git clone https://github.com/radarsat1/dimple.git --depth=1 --branch hugosite hugosite
git clone https://github.com/radarsat1/dimple.git --depth=1 --branch gh-pages pages
# git clone . --branch hugosite hugosite
# git clone . --branch gh-pages pages

(cd hugosite && git submodule init && git submodule update)

# Prepare hugo site with docs
cp -v doc/messages.md hugosite/content/

SITE_HAS_BINARIES=
if ! [ -z $SITE_HAS_BINARIES ]; then
  # Prepare hugo site with current binary
  mkdir -vp pages/static/binaries
  cp -rv $DIMPLE hugosite/static/binaries/

  # Prepare hugo site with current link
  sed -ie "s/dimple-nightly-placeholder-$OS/$DIMPLE/g" hugosite/content/download.md

  # Check binary dependencies
  if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    if [ -e inst/bin/dimple ]; then
      ldd inst/bin/dimple
    elif [ -e inst/bin/dimple.exe ]; then
      true
    fi
  elif [ "$TRAVIS_OS_NAME" = "osx" ]; then
    otool -L inst/bin/dimple
  fi

  # Update hugo site with previous links
  BINARIES=$(grep binaries/dimple pages/download/index.html | sed 's,^.*/binaries/\(.*\)">.*$,\1,')
  for i in $BINARIES; do
    case $i in
      *-linux-*) sed -ie "s/dimple-nightly-placeholder-linux/$i/g" hugosite/content/download.md ;;
      *-mac-*) sed -ie "s/dimple-nightly-placeholder-mac/$i/g" hugosite/content/download.md ;;
      *-win64-*) sed -ie "s/dimple-nightly-placeholder-windows/$i/g" hugosite/content/download.md ;;
    esac
  done
fi

# Empty pages, run hugo, replace contents with static site
rm -rfv pages/*
(cd hugosite && $HUGO && mv -v public/* ../pages/)

# End OS=linux
fi

set +e
set +x
