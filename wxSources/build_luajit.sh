#!/bin/sh
#  Copy the source tree of LuaJIT under the build directory and build LuaJIT
#  Assume that the current directory is the "Project directory" (build-xcode, build-win, etc.)
#  On Windows, patch for LoadLibraryExA is also applied, and 
#  assumes build/lib/win_fopen.o is present.

#  Required environmental variables
#  LUAJIT_VERSION  the version string of LuaJIT
#  TARGET_PLATFORM "MSW" or "MAC"
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

cd build/$LUAJIT_DIR

#  Change the options according to the build settings
mv src/Makefile src/Makefile.orig
sed -e 's/BUILDMODE= mixed/#BUILDMODE= mixed/' \
    -e 's/#BUILDMODE= dynamic/BUILDMODE= dynamic/' \
    src/Makefile.orig >src/Makefile

#  Override windows.h (to patch some system calls that use CP_ACP encoding)
if [ $TARGET_PLATFORM = "MSW" ]; then
  CFLAGS="-I '$PWD/..' $CFLAGS"
  cp -f ../../../wxSources/windows_patch.h ../windows.h
fi

#  We should disable TARGET_ARCH because LuaJIT uses this variable internally
if [ $TARGET_PLATFORM = "MSW" ]; then
  #  On Windows, 'make install' does not work well, so we install the library by hand
  make HOST_CC="$HOST_CC" LDFLAGS="" TARGET_ARCH="" TARGET_SYS="$TARGET_SYS" || exit 1
  mkdir -p ../lib/lua
  cp -f src/lua51.dll ../lib
  cp -f src/luajit.exe ../lib
  rm -rf ../lib/lua/jit
  cp -R src/jit ../lib/lua
elif [ $TARGET_PLATFORM = "MAC" ]; then
  make HOST_CC=$HOST_CC LDFLAGS="" TARGET_ARCH="" TARGET_SYS=$TARGET_SYS PREFIX=$PWD/.. install || exit 1
  install_name_tool -id "@rpath/libluajit-5.1.dylib" ../lib/libluajit-5.1.dylib
#  cp -f src/libluajit-5.1.dylib ../lib
fi

make clean

