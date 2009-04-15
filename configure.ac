AC_PREREQ(2.57)

AC_DEFUN([DIMPLE_VER],[m4_esyscmd([util/version.sh | tr -d '\n'])])
AC_INIT([dimple], [DIMPLE_VER], [dimple@music.mcgill.ca])

CPPFLAGS="$CPPFLAGS -DDIMPLE_VERSION=\\\"DIMPLE_VER\\\""

AM_CONFIG_HEADER([src/config.h])

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE

abs_srcdir=`cd $srcdir; pwd`

# Checks for programs.
AC_LANG_CPLUSPLUS
AC_PROG_CXX
AC_CHECK_PROG(pkgconfig, pkg-config, yes, no)

# Checks for libraries.
AC_CHECK_FUNC([log], [], [AC_CHECK_LIB([m],[log])])

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Some simple configurations depending purely on known target systems
case "$target" in
  *-*-darwin* )
     SYSTEM_LIBS="$SYSTEM_LIBS -framework OpenGL -framework GLUT"
     CHAI_SUFFIX="darwin"
     CHAI_FLAGS="-D_POSIX -DLINUX -D_LINUX -D_MAX_PATH=260"
     # Note: yes the above 'LINUX' flags are correct.
  ;;
  *-*-linux* )
     CHAI_SUFFIX="linux"
     CHAI_FLAGS="-D_POSIX -DLINUX -D_LINUX -D_MAX_PATH=260"
  ;;
  *-*-mingw* )
     CHAI_SUFFIX="mingw"
     CHAI_FLAGS="-D_POSIX -DWIN32 -D_WIN32 -D_MSYS -D_MINGW -DNOMINMAX -D_MAX_PATH=260"
  ;;
esac

# Check for OpenGL-related libraries
# (Skip this test on darwin)
case "$target" in
  *-*-darwin* )

AC_CHECK_HEADER([/System/Library/Frameworks/OpenGL.framework/Headers/gl.h],
  [if ! test -d ${abs_srcdir}/GL; then
    if ! mkdir -v ${abs_srcdir}/GL; then
	  AC_MSG_ERROR([Error creating directory for GL framework symlink.])
    fi
   fi
   if ! test -h ${abs_srcdir}/GL/GL; then
    if ! ln -vs /System/Library/Frameworks/OpenGL.framework/Headers ${abs_srcdir}/GL/GL; then
	  AC_MSG_ERROR([Error creating symlink to GL framework.])
    fi
   fi
   CPPFLAGS="$CPPFLAGS -I${abs_srcdir}/GL"
   LDFLAGS="$LDFLAGS -framework OpenGL"],
  [AC_MSG_ERROR([Couldn't find OpenGL framework.])])

AC_CHECK_HEADER([GL/gl.h], [],
  [AC_MSG_ERROR([Error using symlink to OpenGL framework.])])

AC_CHECK_HEADER([/System/Library/Frameworks/GLUT.framework/Headers/glut.h],
  [if ! test -d ${abs_srcdir}/GLUT; then
    if ! mkdir -v ${abs_srcdir}/GLUT; then
	  AC_MSG_ERROR([Error creating directory for GLUT framework symlink.])
    fi
   fi
   if ! test -h ${abs_srcdir}/GLUT/GL; then
    if ! ln -vs /System/Library/Frameworks/GLUT.framework/Headers ${abs_srcdir}/GLUT/GL; then
	  AC_MSG_ERROR([Error creating symlink to GLUT framework.])
    fi
   fi
   CPPFLAGS="$CPPFLAGS -I${abs_srcdir}/GLUT"
   LDFLAGS="$LDFLAGS -framework GLUT"],
  [AC_MSG_ERROR([Couldn't find GLUT framework.])])

AC_CHECK_HEADER([GL/glut.h], [],
  [AC_MSG_ERROR([Error using symlink to GLUT framework.])])
  ;;
  * )

# OpenGL
AC_CHECK_LIB(GL, [glBegin], [],
  [AC_CHECK_LIB(opengl32, [main], [],
    AC_MSG_ERROR([Couldn't find OpenGL library.]))])

# Note for the above: can't easily check for specific functions under
# MSYS/MingW due to stdcall calling convention.

# GLUT
AC_CHECK_LIB(glut, [glutInit], [],
  [LDFLAGS="$LDFLAGS -L${abs_srcdir}/libdeps/freeglut-2.4.0/src/.libs"
  unset ac_cv_lib_glut_glutInit
  AC_CHECK_LIB(glut, [glutInit],
    [CPPFLAGS="$CPPFLAGS -DFREEGLUT_STATIC"
    CPPFLAGS="$CPPFLAGS -I${abs_srcdir}/libdeps/freeglut-2.4.0/include"
    LIBS="-lglut -lgdi32 -lwinmm -luuid -loleaut32 $LIBS"
    need_glu32=yes],
    [AC_CHECK_LIB(glut32, [glutInit], [],
      AC_MSG_ERROR([Couldn't find GLUT library.]))],
    [-lgdi32 -lwinmm -luuid -loleaut32])])

AC_CHECK_HEADER([GL/freeglut.h],
    [AC_DEFINE(USE_FREEGLUT, 1, [Define this to use freeglut features.])])

# On MingW, the above needs glu32.
if test x${need_glu32} = xyes
then
  AC_CHECK_LIB(glu32, [main], [],
    [AC_MSG_ERROR([Couldn't find glu32 library.])])
fi

  ;;
esac

# Check for bootstrapped libraries.
# Always check in order of preference: (TODO)
# - specified versions
# - libdeps bootstrapped versions
# - local prefix versions
# - system versions

# The "unset" commands are to reset the autoconf cache, otherwise it
# just uses previous value and fails.

# CHAI 3D
CPPFLAGS="$CPPFLAGS $CHAI_FLAGS"
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -I${abs_srcdir}/libdeps/chai3d/include"
AC_CHECK_HEADER(CGenericObject.h, [],
  [CPPFLAGS="$save_CPPFLAGS"
  unset ac_cv_header_CGenericObject_h
  AC_CHECK_HEADER(CGenericObject.h, [],
     [AC_MSG_ERROR([Couldn't find CHAI 3D headers.])])])

save_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS -L${abs_srcdir}/libdeps/chai3d/lib/${CHAI_SUFFIX}"
AC_CHECK_LIB(chai3d_${CHAI_SUFFIX}, [main], [],
  [LDFLAGS="$save_LDFLAGS"
  unset ac_cv_lib_chai3d_${CHAI_SUFFIX}_main
  AC_CHECK_LIB(chai3d_${CHAI_SUFFIX}, [main], [],
    AC_MSG_ERROR([Couldn't find CHAI 3D library.]))])

# ODE
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -I${abs_srcdir}/libdeps/ode-0.10.1/include"
AC_CHECK_HEADER(ode/ode.h, [],
  [CPPFLAGS="$save_CPPFLAGS"
  unset ac_cv_header_ode_ode_h
  AC_CHECK_HEADER(ode/ode.h, [],
     [AC_MSG_ERROR([Couldn't find ODE headers.])])])

save_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS -L${abs_srcdir}/libdeps/ode-0.10.1/ode/src/.libs"
AC_CHECK_LIB(ode, [dBodyCreate], [],
  [LDFLAGS="$save_LDFLAGS"
  unset ac_cv_lib_ode_dBodyCreate
  AC_CHECK_LIB(ode, [dBodyCreate], [],
    AC_MSG_ERROR([Couldn't find ODE library.]))])

# libsamplerate
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -I${abs_srcdir}/libdeps/libsamplerate-0.1.2/src"
AC_CHECK_HEADER(samplerate.h, [],
  [CPPFLAGS="$save_CPPFLAGS"
  unset ac_cv_header_samplerate_h
  AC_CHECK_HEADER(samplerate.h, [],
     [AC_MSG_ERROR([Couldn't find libsamplerate headers.])])])

save_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS -L${abs_srcdir}/libdeps/libsamplerate-0.1.2/src/.libs"
AC_CHECK_LIB(samplerate, [src_new], [],
  [LDFLAGS="$save_LDFLAGS"
  unset ac_cv_lib_samplerate_src_new
  AC_CHECK_LIB(samplerate, [src_new], [],
    AC_MSG_ERROR([Couldn't find libsamplerate library.]))])

# liblo
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -I${abs_srcdir}/libdeps/liblo-0.25"
AC_CHECK_HEADER(lo/lo.h, [],
  [CPPFLAGS="$save_CPPFLAGS"
  unset ac_cv_header_lo_lo_h
  AC_CHECK_HEADER(lo/lo.h, [],
     [AC_MSG_ERROR([Couldn't find liblo headers.])])])

save_LIBS="$LIBS"
save_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS -L${abs_srcdir}/libdeps/liblo-0.25/src/.libs"
AC_CHECK_LIB(lo, [lo_server_new], [],
  [LIBS="-lws2_32 $LIBS"
  unset ac_cv_lib_lo_lo_server_new
  AC_CHECK_LIB(lo, [lo_server_new], [],
    [LDFLAGS="$save_LDFLAGS"
    LIBS="$save_LIBS"
    unset ac_cv_lib_lo_lo_server_new
    AC_CHECK_LIB(lo, [lo_server_new], [],
      AC_MSG_ERROR([Couldn't find liblo library.]))])])

# pthread
AC_CHECK_LIB(pthread, [pthread_create], [],
  [LDFLAGS="$LDFLAGS -L${abs_srcdir}/libdeps/pthreads-w32-2-8-0-release"
  unset ac_cv_lib_pthread_pthread_create
  AC_CHECK_LIB(pthread, [pthread_create],
    [CPPFLAGS="$CPPFLAGS -DPTW32_STATIC_LIB"
    LIBS="-lpthread $LIBS"],
    [AC_MSG_ERROR([Couldn't find pthread library.])],
    [-lws2_32])])

# check if libnifalcon is available: if so, link to it
if test x${ac_cv_prog_pkgconfig} == xyes
then
  save_LDFLAGS="$LDFLAGS"
  LDFLAGS="$save_LDFLAGS `pkg-config --libs libnifalcon`"
  AC_CHECK_LIB(nifalcon, [main], [],
    [LDFLAGS="$save_LDFLAGS"
    unset ac_cv_lib_nifalcon_main
    AC_CHECK_LIB(nifalcon, [main], [],
      [LDFLAGS="$save_LDFLAGS -L${prefix}/lib"
      unset ac_cv_lib_nifalcon_main
      AC_CHECK_LIB(nifalcon, [main], [], [])])])

  AC_CHECK_LIB(nifalcon_comm_libftdi, [main], [], [])
  AC_CHECK_LIB(nifalcon_comm_libusb, [main], [], [])

  case "$target" in
    *-*-darwin* )
    if test x${ac_cv_lib_nifalcon_main} = xyes; then
      LDFLAGS="$LDFLAGS -framework IOKit -framework AppKit"
    fi
    ;;
  esac
else
  AC_MSG_WARN([No pkg-config found, so skipping check for libnifalcon.])
fi

# check for options

# debug mode (also checks for DIMPLE_DEBUG environment variable)
AC_ARG_ENABLE(debug, [  --enable-debug          compile in debug mode],
              [debug=yes])
if test x$debug = xyes || test x$DIMPLE_DEBUG=x1; then
  CPPFLAGS="-DDEBUG $CPPFLAGS"
  if test "$CXXFLAGS" = "-g -O2"; then
    CXXFLAGS="-g -O0 -Wall"
  else
    CXXFLAGS="$CXXFLAGS -g -O0 -Wall"
  fi
fi

AC_CONFIG_FILES([
	Makefile
    src/Makefile
])
AC_OUTPUT()
