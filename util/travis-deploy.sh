#!/bin/bash

set -e
set -x

# HUGO=/snap/hugo/current/bin/hugo
HUGO=hugo

# git fetch --unshallow
util/version.sh

git clone https://github.com/radarsat1/dimple.git --depth=1 --branch hugosite hugosite
git clone https://github.com/radarsat1/dimple.git --depth=1 --branch gh-pages pages
# git clone . --branch hugosite hugosite
# git clone . --branch gh-pages pages

(cd hugosite && git submodule init && git submodule update)

DIMPLE=dimple-$TRAVIS_OS_NAME-`util/version.sh`
case $TRAVIS_OS_NAME in
    osx) OS=mac; util/build-appbundle.sh;;
    linux) OS=linux; [ -z "$MINGW_ON_LINUX" ] && util/build-appimage.sh;;
esac

# Prepare hugo site with docs
cp -v doc/messages.md hugosite/content/

# Prepare hugo site with previous binaries
rm -rfv hugosite/static/binaries/*
mv -v pages/binaries/* hugosite/static/binaries/

# Prepare hugo site with current binary
mkdir -vp pages/static/binaries
if [ -e inst/bin/dimple.exe ]; then
  DIMPLE=dimple-mingw-`util/version.sh`.exe
  OS=windows
  cp -rv inst/bin/dimple.exe hugosite/static/binaries/$DIMPLE
elif [ -e dimple-`util/version.sh`-x86_64.AppImage ]; then
  DIMPLE=dimple-`util/version.sh`-x86_64.AppImage
  cp -rv $DIMPLE hugosite/static/binaries/$DIMPLE
elif [ -e dimple-`util/version.sh`-mac-x86_64.dmg ]; then
  DIMPLE=dimple-`util/version.sh`-mac-x86_64.dmg
  cp -rv $DIMPLE hugosite/static/binaries/$DIMPLE
elif [ -e inst/bin/dimple ]; then
  cp -rv inst/bin/dimple hugosite/static/binaries/$DIMPLE
else
  echo "No installed dimple executable found:"
  find inst
  exit 1
fi

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
    *-mingw-*) sed -ie "s/dimple-nightly-placeholder-windows/$i/g" hugosite/content/download.md ;;
  esac
done

# Empty pages, run hugo, replace contents with static site
rm -rfv pages/*
(cd hugosite && $HUGO && mv -v public/* ../pages/)
