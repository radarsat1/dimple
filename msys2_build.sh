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
choco install msys2
ls /c/
exit 1
