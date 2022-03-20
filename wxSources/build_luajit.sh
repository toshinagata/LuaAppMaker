#!/bin/sh
#  Copy the source tree of LuaJIT under the build directory and build LuaJIT
#  Assume that the current directory is the "Project directory" (build-xcode, build-win, etc.)
#  On Windows, patch for LoadLibraryExA is also applied, and 
#  assumes build/lib/win_fopen.o is present.

#  Required environmental variables
#  LUAJIT_VERSION  the version string of LuaJIT
#  TARGET_PLATFORM "MSW" or "MAC" or "LINUX"
#  TARGET_ARCH "x86_64" or "i686"

LUAJIT_DIR=LuaJIT-$LUAJIT_VERSION

if [ x$CONFIGURATION = x"Debug" ]; then
  DEBUG=1
fi

if [ x$TARGET_PLATFORM = x"MSW" ]; then
  LUAJIT_HOSTCC_FLAGS="HOST_XCFLAGS=-I. -DLUAJIT_OS=LUAJIT_OS_WINDOWS"
  TARGET_SHLDFLAGS="$PWD/build/lib/win_fopen.o"
  TARGET_FLAGS="-static-libgcc"
  TARGET_SYS=Windows
  if [ $TARGET_ARCH = "x86_64" ]; then
    CROSS=x86_64-w64-mingw32-
    TOOL_PREFIX=x86_64-w64-mingw32-
    HOST_CC="gcc"
  else
    CROSS=i686-w64-mingw32-
    TOOL_PREFIX=i686-w64-mingw32-
    HOST_CC="gcc -m32"
  fi
elif [ x$TARGET_PLATFORM = x"MAC" ]; then
  CROSS=
  TOOL_PREFIX=
  HOST_CC="gcc"
  TARGET_SYS="Darwin"
elif [ x$TARGET_PLATFORM = x"LINUX" ]; then
  CROSS=
  TOOL_PREFIX=
  HOST_CC="gcc"
  TARGET_SYS="Linux"
else
  echo "Unknown platform $TARGET_PLATFORM"
  exit 1
fi

export CROSS
export TOOL_PREFIX
export TARGET_SHLDFLAGS
export TARGET_FLAGS
export DEBUG

CFLAGS=""
#if [ $DEBUG = "1" ]; then
#  CFLAGS="-O0 -g"
#else
#  CFLAGS="-O2"
#fi

rm -rf build/$LUAJIT_DIR
cp -R ../$LUAJIT_DIR build/
mkdir -p build/lib

INSTALL_DIR=$PWD/build

cd build/$LUAJIT_DIR

#  Change the options according to the build settings
mv src/Makefile src/Makefile.orig
sed -e 's/BUILDMODE= mixed/#BUILDMODE= mixed/' \
    -e 's/#BUILDMODE= dynamic/BUILDMODE= dynamic/' \
    src/Makefile.orig >src/Makefile

cd src
patch <<'End_of_patch'
--- lj_debug_orig.c	2021-09-03 23:09:40.000000000 +0900
+++ lj_debug.c	2021-09-03 23:55:57.000000000 +0900
@@ -327,6 +327,7 @@
     src++;  /* Skip the `@' */
     if (len >= LUA_IDSIZE) {
       src += len-(LUA_IDSIZE-4);  /* Get last part of file name. */
+      while (*src != 0 && (*src & 0xc0) == 0x80) src++; /* Skip UTF-8 non-boundary chars */
       *out++ = '.'; *out++ = '.'; *out++ = '.';
     }
     strcpy(out, src);
@@ -337,6 +338,7 @@
     strcpy(out, line == ~(BCLine)0 ? "[builtin:" : "[string \""); out += 9;
     if (src[len] != '\0') {  /* Must truncate? */
       if (len > LUA_IDSIZE-15) len = LUA_IDSIZE-15;
+      while (len > 0 && (src[len] & 0xc0) == 0x80) len--; /* Skip UTF-8 non-boundary chars */
       strncpy(out, src, len); out += len;
       strcpy(out, "..."); out += 3;
     } else {
End_of_patch
cd ..

#if [ $DEBUG = "1" ]; then
#  sed -i -e 's/#CCDEBUG= -g/CCDEBUG= -g/' src/Makefile
#fi

#  Override windows.h (to patch some system calls that use CP_ACP encoding)
if [ $TARGET_PLATFORM = "MSW" ]; then
  CFLAGS="-I '$PWD/..' $CFLAGS"
  cp -f ../../../wxSources/windows_patch.h ../windows.h
fi

#  We should disable DESTDIR and TARGET_ARCH because LuaJIT uses this variable internally
if [ $TARGET_PLATFORM = "MSW" ]; then
  #  On Windows, 'make install' does not work well, so we install the library by hand
  make HOST_CC="$HOST_CC" LDFLAGS="" DESTDIR="" TARGET_ARCH="" TARGET_SYS="$TARGET_SYS" || exit 1
  mkdir -p ../lib/lua
  cp -f src/lua51.dll ../lib
  cp -f src/luajit.exe ../lib
  rm -rf ../lib/lua/jit
  cp -R src/jit ../lib/lua
elif [ $TARGET_PLATFORM = "MAC" ]; then
  make HOST_CC=$HOST_CC LDFLAGS="" DESTDIR="" TARGET_ARCH="" TARGET_SYS=$TARGET_SYS PREFIX="$INSTALL_DIR" install || exit 1
  install_name_tool -id "@rpath/libluajit-5.1.dylib" ../lib/libluajit-5.1.dylib
#  cp -f src/libluajit-5.1.dylib ../lib
elif [ $TARGET_PLATFORM = "LINUX" ]; then
  (PATH=$PATH:/usr/sbin:/sbin; make HOST_CC=$HOST_CC LDFLAGS="" DESTDIR="" TARGET_ARCH="" TARGET_SYS=$TARGET_SYS PREFIX="$INSTALL_DIR" install) || exit 1
fi

make clean
