#!/bin/bash

echo "msys2 build script"
pwd
ls
pacman -Ss zip unzip wget cmake patch
which wget
which zip
which unzip
which cmake
which patch
exit 1
