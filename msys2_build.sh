#!/bin/bash

echo "msys2 build script"
echo 0=\"$0\"
pwd
ls
pacman -Ss zip unzip wget cmake patch
which wget
which zip
which unzip
which cmake
which patch
echo == /c/ProgramData/chocolatey/bin && ls /c/ProgramData/chocolatey/bin
echo == /c/ProgramData/chocolatey/lib/mingw/tools/install && ls /c/ProgramData/chocolatey/lib/mingw/tools/install
echo == /c/ProgramData/chocolatey/lib/mingw/tools/install/mingw64/bin && ls /c/ProgramData/chocolatey/lib/mingw/tools/install/mingw64/bin
echo == /c/ && ls /c/
echo == /c/tools && ls /c/tools
echo == /c/tools/msys64 && ls /c/tools/msys64
echo == /c/tools/msys64/bin && ls /c/tools/msys64/bin
choco search msys2
choco install -y msys2
echo == /c/ && ls /c/
exit 1
