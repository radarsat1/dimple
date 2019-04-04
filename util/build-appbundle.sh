#!/bin/bash

# Build an OS X app bundle.

set -e

if ! [ -e inst/bin/dimple ]; then
    echo Expected "'inst/bin/dimple'" to exist, \
         run "'./configure --prefix=\$PWD/inst && make install'".
    exit 1
fi

set -x

otool -L inst/bin/dimple

# copy extra files (tests, examples) into the AppDir
mkdir -vp dmg/dimple.app/Contents/MacOs
mkdir -vp dmg/dimple.app/Contents/Resources
cp -rv test maxmsp textures dmg/
cp -rv inst/share/doc/dimple dmg/doc
markdown dmg/doc/README >README.html

cp -v inst/bin/dimple dmg/dimple.app/Contents/MacOs/
cp -v icon/dimple_sphere.icns dmg/dimple.app/Contents/Resources/

# TODO: device libraries from chai3d

NAME=dimple
EXECUTABLE_NAME=dimple
VERSION=$(util/version.sh)
MACOSX_DEPLOYMENT_TARGET=10.8
ICON_FILE=dimple_sphere.icns

cat >dmg/dimple.app/Contents/Info.plist <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>English</string>
	<key>CFBundleExecutable</key>
	<string>${EXECUTABLE_NAME}</string>
	<key>CFBundleIconFile</key>
	<string>${ICON_FILE}</string>
	<key>CFBundleIdentifier</key>
	<string>com.dimple</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>${NAME}</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleShortVersionString</key>
	<string>${VERSION}</string>
	<key>CFBundleSignature</key>
	<string>????</string>
	<key>CFBundleVersion</key>
	<string>${VERSION}</string>
	<key>LSMinimumSystemVersion</key>
	<string>${MACOSX_DEPLOYMENT_TARGET}</string>
</dict>
</plist>
EOF

echo -n >dmg/dimple.app/Contents/PkgInfo 'APPL????'

# Build the dmg
DMG_VOL_NAME=${NAME}-${VERSION}
DMG_FILE=${DMG_VOL_NAME}-mac-x86_64.dmg
DMG_TMP_FILE=${DMG_VOL_NAME}-mac-x86_64-tmp.dmg
DMG_SIZE=`du -sh "dmg" | sed 's/\([0-9\.]*\)M\(.*\)/\1/'`
DMG_SIZE=`echo "${DMG_SIZE} + 10.0" | bc | awk '{print int($1+0.5)}'`

hdiutil create -srcfolder dmg -volname "${DMG_VOL_NAME}" -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDRW -size ${DMG_SIZE}M "$DMG_TMP_FILE"
hdiutil convert "${DMG_TMP_FILE}" -format UDZO -imagekey zlib-level=9 -o "${DMG_FILE}"
rm -fv "${DMG_TMP_FILE}"
