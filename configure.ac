AC_PREREQ(2.57)

AC_DEFUN([DIMPLE_VER],[m4_esyscmd([util/version.sh | tr -d '\n'])])
AC_INIT([dimple], DIMPLE_VER, [dimple@music.mcgill.ca])

CPPFLAGS="$CPPFLAGS -DDIMPLE_VERSION=\\\"DIMPLE_VER\\\""

AM_CONFIG_HEADER([src/config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([bootstrap.sh])

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

abs_srcdir=`cd $srcdir; pwd`

echo "CC is ${CC}"
echo "CXX is ${CXX}"

# Checks for programs.
AC_LANG_CPLUSPLUS
AC_PROG_CXX
AC_CHECK_PROG(pkgconfig, pkg-config, yes, no)
AX_CXX_COMPILE_STDCXX_11

# Checks for libraries.
AC_CHECK_FUNC([log], [], [AC_CHECK_LIB([m],[log])])
AC_CHECK_FUNC([getopt_long], [],
  [AC_MSG_ERROR([Couldn't find getopt_long])])
AC_CHECK_LIB([dl],[dlopen])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADER([getopt.h], [],
  [AC_MSG_ERROR([Couldn't find getopt.h.])])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Some simple configurations depending purely on known target systems
CHAI_ARCH="`uname -m`"
case "$target" in
  *-*-darwin* )
     LIBS="$LIBS -framework OpenGL -framework GLUT -framework CoreAudio"
     CHAI_OS=mac
     if test -z "${CC}"; then
       CHAI_CC=clang;
     else
       CHAI_CC="${CC}";
     fi
  ;;
  *-*-linux* )
     CHAI_OS=lin
     if test -z "${CC}"; then
       CHAI_CC=cc;
     else
       CHAI_CC="${CC}";
     fi
  ;;
  *-*-mingw* )
     CHAI_OS="mingw" # to be tested
  ;;
esac
CHAI_SUFFIX="${CHAI_OS}-${CHAI_ARCH}-${CHAI_CC}"

# Check for OpenGL-related libraries
# (Skip this test on darwin)
case "$target" in
  *-*-darwin* )

AC_CHECK_HEADER([/System/Library/Frameworks/OpenGL.framework/Headers/gl.h],
  [echo abs_srcdir=\"$abs_srcdir\" PWD=\"$PWD\";
   pwd;
   echo ===;
   ls -l;
   echo ===;
   whoami;
   ls -ld ${abs_srcdir};
   ls -l ${abs_srcdir};
   echo ===;
   ls -l ${abs_srcdir}/GL;
   echo ===;
   if ! test -d ${abs_srcdir}/GL; then
    if ! mkdir -v ${abs_srcdir}/GL; then
	  AC_MSG_ERROR([Error creating directory for GL framework symlink.])
    fi
   fi
   echo ===;
   ls -l ${abs_srcdir}/GL;
   echo ===;
   if ! test -h ${abs_srcdir}/GL/GL; then
    if ! ln -vs /System/Library/Frameworks/OpenGL.framework/Headers ${abs_srcdir}/GL/GL; then
	  AC_MSG_ERROR([Error creating symlink to GL framework.])
    fi
   fi
   CPPFLAGS="$CPPFLAGS -I${abs_srcdir}/GL"
   LIBS="$LIBS -framework OpenGL"],
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
   LIBS="$LIBS -framework GLUT"],
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

# GLU
AC_CHECK_LIB(GLU, [gluLookAt], [],
  [AC_CHECK_LIB(glu32, [main], [],
    AC_MSG_ERROR([Couldn't find GLU library.]))])

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

# pthread
AX_PTHREAD(
  [CPPFLAGS="$CPPFLAGS $PTHREAD_CFLAGS"
  LDFLAGS="$LDFLAGS $PTHREAD_CFLAGS"
  LIBS="$PTHREAD_LIBS $LIBS"],
  [LDFLAGS="$LDFLAGS -L${abs_srcdir}/libdeps/pthreads-w32-2-8-0-release"
  unset ac_cv_lib_pthread_pthread_create
  AC_CHECK_LIB(pthread, [pthread_create],
    [CPPFLAGS="$CPPFLAGS -DPTW32_STATIC_LIB"
    LIBS="-lpthread $LIBS"],
    [AC_MSG_ERROR([Couldn't find pthread library.])],
    [-lws2_32])])

# CHAI 3D
CHAI_DIR=${abs_srcdir}/libdeps/chai3d-3.2.0
CHAI_LIBDIR=${CHAI_DIR}/lib/release/${CHAI_SUFFIX}
CHAI_INCDIRS="-I${CHAI_DIR}/src -I${CHAI_DIR}/external/Eigen -I${CHAI_DIR}/external/glew/include"
CPPFLAGS="$CPPFLAGS $CHAI_FLAGS"
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS ${CHAI_INCDIRS}"
AC_CHECK_HEADER(chai3d.h, [],
  [CPPFLAGS="$save_CPPFLAGS"
  unset ac_cv_header_chai3d_h
  AC_MSG_ERROR([Couldn't find CHAI 3D headers.])])

save_LIBS="$LIBS"
LDFLAGS="$LDFLAGS -L${CHAI_LIBDIR}"
# In case CHAI is using OpenHaptics
if test -f /usr/include/HL/hl.h; then
  LIBS="$LIBS -lHD -lHL"
fi
# In case CHAI is using DHD
echo ls ${CHAI_DIR}/external/DHD/lib
ls ${CHAI_DIR}/external/DHD/lib
if test -f ${CHAI_DIR}/external/DHD/lib/${CHAI_OS}-${CHAI_ARCH}/libdrd.a; then
  LDFLAGS="$LDFLAGS -L${CHAI_DIR}/external/DHD/lib/${CHAI_OS}-${CHAI_ARCH}"
  AC_CHECK_LIB([usb-1.0], [libusb_init])
  AC_CHECK_LIB([drd], [dhdOpenID])
  # TODO: compile small chai program that instantiates a haptic
  # device, check what libraries it requires
fi
echo ls ${CHAI_LIBDIR}
if ! [ ls ${CHAI_LIBDIR} ]; then
  find libdeps
fi
AC_CHECK_LIB(chai3d, [main], [],
  [LIBS="$save_LIBS"
  unset ac_cv_lib_chai3d_main
  AC_CHECK_LIB(chai3d, [main], [],
    AC_MSG_ERROR([Couldn't find CHAI 3D library.]))])

# ODE
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -I${abs_srcdir}/libdeps/ode-0.13.1/include"
AC_CHECK_HEADER(ode/ode.h, [],
  [CPPFLAGS="$save_CPPFLAGS"
  unset ac_cv_header_ode_ode_h
  AC_CHECK_HEADER(ode/ode.h, [],
     [AC_MSG_ERROR([Couldn't find ODE headers.])])])

save_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS -L${abs_srcdir}/libdeps/ode-0.13.1/ode/src/.libs"
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
CPPFLAGS="$CPPFLAGS -I${abs_srcdir}/libdeps/liblo-0.29"
AC_CHECK_HEADER(lo/lo.h, [],
  [CPPFLAGS="$save_CPPFLAGS"
  unset ac_cv_header_lo_lo_h
  AC_CHECK_HEADER(lo/lo.h, [],
     [AC_MSG_ERROR([Couldn't find liblo headers.])])])

save_LIBS="$LIBS"
save_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS -L${abs_srcdir}/libdeps/liblo-0.29/src/.libs"
AC_CHECK_LIB(lo, [lo_server_new], [],
  [LIBS="-lws2_32 $LIBS"
  unset ac_cv_lib_lo_lo_server_new
  AC_CHECK_LIB(lo, [lo_server_new], [],
    [LDFLAGS="$save_LDFLAGS"
    LIBS="$save_LIBS"
    unset ac_cv_lib_lo_lo_server_new
    AC_CHECK_LIB(lo, [lo_server_new], [],
      AC_MSG_ERROR([Couldn't find liblo library.]))])])

# check if libnifalcon is available: if so, link to it
if test x${ac_cv_prog_pkgconfig} == xyes
then
  save_LIBS="$LIBS"
  LIBS="$save_LIBS `pkg-config --libs libnifalcon`"
  AC_CHECK_LIB(nifalcon, [main], [],
    [LIBS="$save_LIBS"
    unset ac_cv_lib_nifalcon_main
    AC_CHECK_LIB(nifalcon, [main], [],
      [LIBS="$save_LIBS -L${prefix}/lib"
      unset ac_cv_lib_nifalcon_main
      AC_CHECK_LIB(nifalcon, [main], [], [])])])

  AC_CHECK_LIB(nifalcon_comm_libftdi, [main], [], [])
  AC_CHECK_LIB(nifalcon_comm_libusb, [main], [], [])

  case "$target" in
    *-*-darwin* )
    if test x${ac_cv_lib_nifalcon_main} = xyes; then
      LIBS="$LIBS -framework IOKit -framework AppKit"
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
if test x$debug = xyes || test x$DIMPLE_DEBUG = x1; then
  CPPFLAGS="-DDEBUG $CPPFLAGS"
  if test "$CXXFLAGS" = "-g -O2"; then
    CXXFLAGS="-g -O0 -Wall"
  else
    CXXFLAGS="$CXXFLAGS -g -O0 -Wall"
  fi
fi

# queue size
AC_ARG_WITH(queue-size,
  [AS_HELP_STRING([--with-queue-size],[default size of message queues in kB, default 64])],
  [queue_size=${withval}], [queue_size=64])
AC_DEFINE_UNQUOTED(DEFAULT_QUEUE_SIZE, ${queue_size},
                   [Default size of message queue in kB])

# use messages queues
AC_ARG_ENABLE(queues,
  [AS_HELP_STRING([--disable-queues],[don't use messages queues, instead use
                                      UDP for inter-thread communication])],
  [use_queues=$enableval], [use_queues=yes])

if test x$use_queues = xyes; then
  AC_DEFINE(USE_QUEUES, [1],
    [Define to enable use of message queues for inter-thread communication.])
fi

AC_CONFIG_FILES([
	Makefile
    src/Makefile
])
AC_OUTPUT()
