#!/bin/sh

cd src

# Customize to the Flext build.{bat/sh} path

case $(uname) in
	CYGWIN*)
		FLEXTBUILD="$(cygpath -d "$HOME/MyDocu~1/projects/pd/pure-data/externals/grill/flext/build.bat")"
        VCVARS=/mnt/c/Program\ Files/Microsoft\ Visual\ Studio\ .NET\ 2003/Vc7/bin/vcvars32.bat
        VCVARS=$(cygpath -d "$VCVARS")
        cd $(cygpath -u "$(cygpath -d "$PWD")")
		cmd /c "call $VCVARS && $FLEXTBUILD pd msvc $1"
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
