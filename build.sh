#!/bin/sh

cd src

# Customize to the Flext build.{bat/sh} path
FLEXTBUILD=$(cygpath -d $HOME/MyDocu~1/projects/puredata/externals/grill/flext/build.bat)

cmd /c "$FLEXTBUILD pd msvc $1"
