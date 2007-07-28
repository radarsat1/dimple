#!/bin/sh

cd src

# Customize to the Flext build.{bat/sh} path

case $(uname) in
	CYGWIN*)
		FLEXTBUILD="$(cygpath -d "$HOME/MyDocu~1/projects/puredata/externals/grill/flext/build.bat")"
		cmd /c "$FLEXTBUILD pd msvc $1"
		exit
		;;
	Linux*)
		FLEXTBUILD="$HOME/projects/puredata/externals/grill/flext/build.sh"
		;;
	Darwin*)
		FLEXTBUILD="$HOME/projects/puredata/externals/grill/flext/build.sh"
		;;
	*)
		echo "Operating system not supported."
		exit
		;;
esac

bash $FLEXTBUILD pd gcc $1
