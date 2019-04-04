#!/bin/bash

set -e
set -x

NAME=dimple
EXECUTABLE_NAME=${NAME}.exe
VERSION=$(util/version.sh)
AUTHOR="Stephen Sinclair"

if ! [ -d nsis ]; then
    mkdir -p nsis
fi

# copy executable
cp -v inst/bin/${EXECUTABLE_NAME} nsis/
# TODO: device libraries from chai3d

# copy doc files
cp -rv inst/share/doc/dimple nsis/doc
markdown nsis/doc/README >nsis/README.html

# copy extra files (tests, examples)
cp -rv test maxmsp textures nsis/

cd nsis

if ! [ -e nsis-3.04.zip ]; then
    wget 'https://sourceforge.net/projects/nsis/files/NSIS 3/3.04/nsis-3.04.zip'
fi
if ! [ -d nsis-3.04 ]; then
    unzip nsis-3.04.zip
fi

cat >installer.nsi <<EOF
Name "${NAME}"

; define name of installer
OutFile "${NAME}-${VERSION}-win64-installer.exe"

; define installation directory
InstallDir \$PROGRAMFILES64\\${NAME}

; Prompt the user to enter a directory
DirText "This will install dimple on your computer. Choose a directory."

; For removing Start Menu shortcut in Windows 7
RequestExecutionLevel admin

; start default section
Section
EOF

find ${EXECUTABLE_NAME} README.html doc test maxmsp textures -type f\
     -printf "    SetOutPath \$INSTDIR/%h\\n    File %p\\n" \
    | tr '/' '\\' \
    | sed 's,@oname,/oname,' \
          >>installer.nsi

cat >>installer.nsi <<EOF

    ; create the uninstaller
    WriteUninstaller "\$INSTDIR\\uninstall.exe"

    ; create a shortcut named "new shortcut" in the start menu programs directory
    ; point the new shortcut at the program
    CreateShortCut "\$SMPROGRAMS\\dimple.lnk" "\$INSTDIR\\${EXECUTABLE_NAME}"
SectionEnd

function un.onInit
	SetShellVarContext all

	#Verify the uninstaller - last chance to back out
	MessageBox MB_OKCANCEL "Permanantly remove ${NAME}?" IDOK next
		Abort
	next:
functionEnd

; uninstaller section start
Section "uninstall"

    ; first, delete the uninstaller
    Delete "\$INSTDIR\\uninstall.exe"

    ; second, remove the link from the start menu
    Delete "\$SMPROGRAMS\\dimple.lnk"
EOF

find ${EXECUTABLE_NAME} README.html doc test maxmsp textures -type f\
     -printf "    Delete \$INSTDIR/%p\\n" \
    | tr '/' '\\' \
    | sed 's,@oname,/oname,' \
          >>installer.nsi

find ${EXECUTABLE_NAME} README.html doc test maxmsp textures -type d\
     -printf "    RmDir \$INSTDIR/%p\\n" \
    | sort -r \
    | tr '/' '\\' \
    | sed 's,@oname,/oname,' \
          >>installer.nsi

cat >>installer.nsi <<EOF
    RmDir "\$INSTDIR"

; uninstaller section end
SectionEnd
EOF

env DISPLAY= wine nsis-3.04/Bin/makensis.exe installer.nsi
