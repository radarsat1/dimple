#!/bin/bash

set -e

if ! [ -e inst/bin/dimple ]; then
    echo Expected "'inst/bin/dimple'" to exist, \
         run "'./configure --prefix=\$PWD/inst && make install'".
    exit 1
fi

set -x

LD_URL=https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage

# To add if linuxdeploy ever gets a stable release.
# LD_MD5=ddfb0a3b367ab81efd2e2ca585efd67a
# if ! (echo $LD_MD5 linuxdeploy | md5sum -c); then
#     wget -O linuxdeploy $LD_URL
# fi
# echo $LD_MD5 linuxdeploy | md5sum -c

if ! [ -e linuxdeploy ]; then
    wget -O linuxdeploy $LD_URL
    chmod +x linuxdeploy
fi

# requires imagemagick!
convert icon/dimple_sphere.png -resize 128 icon/dimple.png

# copy doc files
mkdir -pv AppDir/usr/share/doc
cp -rv inst/share/doc/dimple AppDir/usr/share/doc/
markdown AppDir/usr/share/doc/dimple/README >AppDir/usr/share/doc/dimple/README.html

# copy extra files (tests, examples) into the AppDir
mkdir -vp AppDir/usr/share/dimple/
cp -rv test maxmsp textures AppDir/usr/share/dimple/

# TODO: device libraries from chai3d

# remove the man page because it includes a bad path and is not really
# accessible through man anyways.
rm -fv AppDir/usr/share/man/man1/dimple.1.gz

export VERSION=$(util/version.sh)

./linuxdeploy \
    --appdir AppDir \
    -e inst/bin/dimple \
    -i icon/dimple.png \
    --create-desktop-file \
    --output appimage
