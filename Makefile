#
#   Makefile for LuaAppMaker
#
#  Created by Toshi Nagata on 2018/10/02.
#  Copyright 2018 Toshi Nagata. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#

PWD ?= $(shell pwd)

#  wxLua project base directory
WXLUA_DIR = $(PWD)/..

#  LuaJIT directory
LUAJIT_VERSION = 2.1.0-beta3
#LUAJIT_VERSION = 2.0.5

LUAJIT_DIR = $(PWD)/../LuaJIT-$(LUAJIT_VERSION)

#  wxWidgets base directory
#  (Depends on your installation)
WX_DIR = $(PWD)/../../wxWidgets-3.1.5

#  Application name
APPNAME = LuaAppMaker

ifeq (,$(findstring Windows,$(OS)))
  HOST_SYS:= $(shell uname -s)
else
  HOST_SYS= Windows
endif

ifeq ($(TARGET_PLATFORM),MSW)
  TARGET_SYS= Windows
else
  ifeq ($(TARGET_PLATFORM),MAC)
    TARGET_SYS= Darwin
  else
    ifeq ($(TARGET_PLATFORM),LINUX)
      TARGET_SYS= Linux
    endif
  endif
endif

#  Target platform and cross compile flag
ifneq ($(TARGET_SYS),$(HOST_SYS))
  ifeq ($(TARGET_SYS),Windows)
    ifeq ($(TARGET_ARCH),x86_64)
      CROSS=mingw64
    else
      CROSS=mingw32
    endif
  endif
else
  CROSS=
endif

#  Object files

#  wxLua
ifeq ($(TARGET_PLATFORM),LINUX)
  WXWEBVIEW_O =
else
  WXWEBVIEW_O = wxwebview_bind.o
endif
WXLUA_DEBUG_O = wxldebug.o wxlstack.o
WXLUA_DEBUGGER_O = wxldserv.o wxldtarg.o wxlsock.o wxluadebugger_bind.o
WXLUA_O = lbitlib.o wxlbind.o wxlcallb.o wxlconsole.o wxllua.o wxlobject.o wxlstate.o wxlua_bind.o
WXBIND_O = wxadv_bind.o wxadv_wxladv.o wxaui_bind.o wxbase_base.o wxbase_bind.o wxbase_config.o \
  wxbase_data.o wxbase_datetime.o wxbase_file.o wxcore_appframe.o wxcore_bind.o wxcore_clipdrag.o \
  wxcore_controls.o wxcore_core.o wxcore_defsutils.o wxcore_dialogs.o wxcore_event.o wxcore_gdi.o \
  wxcore_geometry.o wxcore_graphics.o wxcore_help.o wxcore_image.o wxcore_mdi.o wxcore_menutool.o wxcore_picker.o \
  wxcore_print.o wxcore_sizer.o wxcore_windows.o wxcore_wxlcore.o wxgl_bind.o wxhtml_bind.o \
  wxhtml_wxlhtml.o wxmedia_bind.o wxnet_bind.o wxpropgrid_bind.o wxrichtext_bind.o wxstc_bind.o \
  $(WXWEBVIEW_O) wxxml_bind.o wxxrc_bind.o
WXLUAAPP_O = LuaAppMaker.o ConsoleFrame.o lua_addition.o ProgressDialog.o  #  uri.o
LUAGL_O = luagl_const.o luagl_util.o luagl.o luaglu.o
WXLUA_ALL_O = $(addprefix wxlua/wxLua/modules/wxlua/debug/,$(WXLUA_DEBUG_O)) \
  $(addprefix wxlua/wxLua/modules/wxlua/debugger/,$(WXLUA_DEBUGGER_O)) \
  $(addprefix wxlua/wxLua/modules/wxlua/,$(WXLUA_O)) \
  $(addprefix wxlua/wxLua/modules/wxbind/src/,$(WXBIND_O)) \
  $(addprefix wxSources/,$(WXLUAAPP_O)) \

OBJECTS = $(WXLUA_ALL_O)

SUBDIRS = wxlua/wxLua/modules/wxlua/debug wxlua/wxLua/modules/wxlua/debugger wxlua/wxLua/modules/wxbind/src wxSources

#  wx libraries
WXLIB_LIST = core,base,gl,adv

ifeq ($(TARGET_PLATFORM),MSW)
 CPP_EXTRA_FLAGS = -I$(LUAJIT_DIR)/src -I../wxlua/wxLua -I../wxlua/wxLua/modules -I../wxlua/wxLua/modules/wxbind/include -I../wxlua/wxLua/modules/wxbind/setup -DLUA_COMPAT_MODULE -fpermissive
 ifeq ($(CROSS),)
  ifeq ($(TARGET_ARCH),i686)
   BUILD_DIR = build-win32
   TOOL_PREFIX = i686-w64-mingw32-
   PATH_PREFIX=/mingw32/bin/
   LIB_SUFFIX = -3.1-i686-w64-mingw32
   HOST_CC = "gcc"
   LUAJIT_HOSTCC_FLAGS = "HOST_XCFLAGS=-I. -DLUAJIT_OS=LUAJIT_OS_WINDOWS"
  else
   BUILD_DIR = build-win
   TOOL_PREFIX = x86_64-w64-mingw32-
   PATH_PREFIX=/mingw64/bin/
   LIB_SUFFIX = -3.1-x86_64-w64-mingw32
  endif
 else
  PATH_PREFIX=/usr/local/homebrew/bin/
  ifeq ($(TARGET_ARCH),i686)
   BUILD_DIR = build-win32
   TOOL_PREFIX = i686-w64-mingw32-
   LIB_SUFFIX = -3.1-i686-w64-mingw32
   HOST_CC = "gcc -m32"
   LUAJIT_HOSTCC_FLAGS = "HOST_XCFLAGS=-I. -DLUAJIT_OS=LUAJIT_OS_WINDOWS"
  else
   ifeq ($(TARGET_ARCH),x86_64)
    BUILD_DIR = build-win
    TOOL_PREFIX = x86_64-w64-mingw32-
    LIB_SUFFIX = -3.1-x86_64-w64-mingw32
    HOST_CC = "gcc"
   endif
  endif
  WINE_PATH=/Applications/EasyWine.app/Contents/Resources/wine/bin
 endif
 WX_LIB_DIR = $(WX_DIR)/$(BUILD_DIR)/lib
 WX_ARCH_DIR = $(WX_LIB_DIR)/wx/include/$(TOOL_PREFIX)msw-unicode-static-3.1
 WX_CPPFLAGS = -isystem $(WX_ARCH_DIR) -isystem $(WX_DIR)/include -D_LARGEFILE_SOURCE=unknown -D__WXMSW__
 WX_LDFLAGS = -L$(WX_LIB_DIR) -Wl,--subsystem,windows -mwindows $(WX_LIB_DIR)/libwx_mswu_gl$(LIB_SUFFIX).a -lopengl32 -lglu32 $(WX_LIB_DIR)/libwx_mswu$(LIB_SUFFIX).a -lwxregexu$(LIB_SUFFIX) -lwxexpat$(LIB_SUFFIX) -lwxtiff$(LIB_SUFFIX) -lwxjpeg$(LIB_SUFFIX) -lwxpng$(LIB_SUFFIX) -lwxzlib$(LIB_SUFFIX) -lwxscintilla$(LIB_SUFFIX) -lrpcrt4 -loleaut32 -lole32 -luuid -luxtheme -lwinspool -lwinmm -lshell32 -lshlwapi -lcomctl32 -lcomdlg32 -ladvapi32 -lversion -lwsock32 -lgdi32 -loleacc -lwinhttp -limm32
 LD_EXTRA_FLAGS = -static
 EXECUTABLE = _$(APPNAME).exe_
 FINAL_EXECUTABLE = $(APPNAME).exe
 EXE_SUFFIX = .exe
 PRODUCT_DIR = $(APPNAME)
 PRODUCT = $(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
 EXTRA_OBJECTS = wxSources/win_resources.o
 LUAJIT_LIB = build/lib/lua51.dll
 WIN_FOPEN_O = build/lib/win_fopen.o
 LUAJIT_LDFLAGS = build/lib/lua51.dll
endif

ifeq ($(TARGET_PLATFORM),LINUX)
 CPP_EXTRA_FLAGS = -I$(LUAJIT_DIR)/src -I../wxlua/wxLua -I../wxlua/wxLua/modules -I../wxlua/wxLua/modules/wxbind/include -I../wxlua/wxLua/modules/wxbind/setup -DLUA_COMPAT_MODULE -fpermissive
 ifeq ($(CROSS),)
  ifeq ($(TARGET_ARCH),i686)
   BUILD_DIR = build-debian
   TOOL_PREFIX =
   PATH_PREFIX =
   LIB_SUFFIX = -3.1
   HOST_CC = "gcc"
   APPIMAGE_TOOL_ARCH = i686
   APPIMAGE_PRODUCT_ARCH = i386
   #LUAJIT_HOSTCC_FLAGS = "HOST_XCFLAGS=-I. -DLUAJIT_OS=LUAJIT_OS_WINDOWS"
  else
   BUILD_DIR = build-debian64
   TOOL_PREFIX =
   PATH_PREFIX=
   LIB_SUFFIX = -3.1
   APPIMAGE_TOOL_ARCH = x86_64
   APPIMAGE_PRODUCT_ARCH = x86_64
  endif
 else
  PATH_PREFIX=/usr/local/homebrew/bin/
  ifeq ($(TARGET_ARCH),i686)
   BUILD_DIR = build-win32
   TOOL_PREFIX = i686-w64-mingw32-
   LIB_SUFFIX = -3.0-i686-w64-mingw32
   HOST_CC = "gcc -m32"
   #LUAJIT_HOSTCC_FLAGS = "HOST_XCFLAGS=-I. -DLUAJIT_OS=LUAJIT_OS_WINDOWS"
  else
   ifeq ($(TARGET_ARCH),x86_64)
    BUILD_DIR = build-win
    TOOL_PREFIX = x86_64-w64-mingw32-
    LIB_SUFFIX = -3.0-x86_64-w64-mingw32
    HOST_CC = "gcc"
   endif
  endif
 endif
 WX_LIB_DIR = $(WX_DIR)/$(BUILD_DIR)/lib
 WX_ARCH_DIR = $(WX_LIB_DIR)/wx/include/gtk3-unicode-static-3.1
 WX_CPPFLAGS = -I$(WX_ARCH_DIR) -I$(WX_DIR)/include -D_FILE_OFFSET_BITS=64 -D__WXGTK__ -pthread -DwxLUA_USEBINDING_WXWEBVIEW=0
 WX_LDFLAGS = -fPIC -L$(WX_LIB_DIR) -pthread $(WX_LIB_DIR)/libwx_gtk3u_gl$(LIB_SUFFIX).a -lGL -Wl,-Bstatic,-lGLU,-Bdynamic -lEGL -lwayland-egl -lwayland-client $(WX_LIB_DIR)/libwx_gtk3u$(LIB_SUFFIX).a -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lgthread-2.0 -pthread -lglib-2.0 -lX11 -lXxf86vm -lSM -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lXtst -lpangoft2-1.0 -lpango-1.0 -lgobject-2.0 -lglib-2.0 -lfontconfig -lfreetype -lpng -lz -lwxregexu$(LIB_SUFFIX) -lwxtiff$(LIB_SUFFIX) -lwxjpeg$(LIB_SUFFIX) -lwxscintilla$(LIB_SUFFIX) -lz -lexpat -lpng -lz -ldl -lm
     # -lwebkit2gtk-4.0 -ljavascriptcoregtk-4.0
 LD_EXTRA_FLAGS = -L$(PWD)/build/lib -Wl,-rpath='$$ORIGIN/lib' -Wl,-export-dynamic
 LD_EXTRA_FLAGS_WEBVIEW_SO = -L$(PWD)/build/lib -Wl,-rpath='$$ORIGIN/..'
 EXECUTABLE = _$(APPNAME)_
 FINAL_EXECUTABLE = $(APPNAME)
 EXE_SUFFIX =
 PRODUCT_DIR = $(APPNAME)
 PRODUCT = $(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
 LUAJIT_LIB = build/lib/libluajit-5.1.so.2
 LUAJIT_LDFLAGS = -lluajit-5.1
endif

CPP = $(PATH_PREFIX)$(TOOL_PREFIX)g++
CC = $(PATH_PREFIX)$(TOOL_PREFIX)gcc
AR = $(PATH_PREFIX)$(TOOL_PREFIX)ar
RANLIB = $(PATH_PREFIX)$(TOOL_PREFIX)ranlib

ifeq ($(MAKECMDGOALS),debug)
 DEBUG = 1
endif

ifeq ($(CONFIGURATION),Debug)
 DEBUG = 1
endif

ifeq ($(DEBUG),1)
 DESTPREFIX = build/debug
 COPT = -O0 -g
else
 DESTPREFIX = build/release
 COPT = -O2 -g
endif
MAKEDIR = $(PWD)
DESTDIR = $(PWD)/$(DESTPREFIX)
CFLAGS = $(CPPFLAGS) $(COPT) $(CPP_EXTRA_FLAGS) $(WX_CPPFLAGS)
LDFLAGS = $(LD_EXTRA_FLAGS) $(WX_LDFLAGS)

export CFLAGS
export LDFLAGS
export DESTDIR
export CC
export CPP
export AR
export TARGET_PLATFORM
export TARGET_ARCH
export RANLIB
export LUAJIT_VERSION
export PATH_PREFIX

release: all

debug: all

all: make_dir $(DESTPREFIX)/$(PRODUCT)

make_dir:
	echo "Creating $(DESTPREFIX)/$(PRODUCT)"
	for i in $(SUBDIRS); do mkdir -p $(DESTPREFIX)/$$i; done

ifeq ($(TARGET_PLATFORM),MSW)
RESOURCE = wxSources/$(APPNAME)_rc.o
$(DESTPREFIX)/$(RESOURCE) : $(WXLUA_DIR)/wxSources/$(APPNAME).rc
	$(PATH_PREFIX)$(TOOL_PREFIX)windres -i $< -o $@ -I$(WX_DIR)/include
endif

depend: cleandep $(DESTPREFIX) $(OBJECTS:%.o=$(DESTPREFIX)/%.d) $(EXTRA_OBJECTS:%.o=$(DESTPREFIX)/%.d)
	cat $(DESTPREFIX)/*.d > build/Makefile.depend

$(DESTPREFIX): make_dir

cleandep:
	rm -f build/Makefile.depend

-include build/Makefile.depend

$(DESTPREFIX)/%.d : $(WXLUA_DIR)/%.cpp
	$(CPP) -MM $< >$@ $(subst -arch ppc,,$(CFLAGS))

$(DESTPREFIX)/%.d : $(WXLUA_DIR)/%.c
	$(CC) -MM $< >$@ $(subst -arch ppc,,$(CFLAGS))

$(DESTPREFIX)/%.o : $(WXLUA_DIR)/%.cpp
	$(CPP) -c $< -o $@ $(CFLAGS)

$(DESTPREFIX)/%.o : $(WXLUA_DIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

ifeq ($(TARGET_PLATFORM),MSW)
$(WIN_FOPEN_O) : $(WXLUA_DIR)/wxSources/win_fopen.c
	@mkdir -p build/lib
	$(CC) -c $< -o $@ $(CFLAGS)
endif

ifeq ($(TARGET_PLATFORM),MSW)
$(LUAJIT_LIB) : $(WIN_FOPEN_O)
	(PATH=$(PATH_PREFIX):$(PATH); sh ../wxSources/build_luajit.sh) || exit 1
endif

ifeq ($(TARGET_PLATFORM),LINUX)
$(LUAJIT_LIB) :
	(PATH=$(PATH_PREFIX):$(PATH); sh ../wxSources/build_luajit.sh) || exit 1
endif

ALL_OBJECTS = $(OBJECTS) $(EXTRA_OBJECTS) $(RESOURCE)
DESTOBJECTS = $(addprefix $(DESTPREFIX)/,$(ALL_OBJECTS))
$(DESTPREFIX)/$(EXECUTABLE) : $(LUAJIT_LIB) $(DESTPREFIX) $(DESTOBJECTS)
	sh ../wxSources/record_build_date.sh >$(DESTPREFIX)/buildInfo.c
	$(CC) -c $(DESTPREFIX)/buildInfo.c -o $(DESTPREFIX)/buildInfo.o $(CFLAGS)
	$(CPP) -o $@ $(DESTOBJECTS) $(DESTPREFIX)/buildInfo.o $(WIN_FOPEN_O) $(CFLAGS) $(LDFLAGS) $(LUAJIT_LDFLAGS)

ifeq ($(TARGET_PLATFORM),LINUX)
WEBVIEW_SO = $(DESTPREFIX)/wxwebview.so
WXLUA_SRC_PATH = wxlua/wxLua/modules/wxbind/src
$(DESTPREFIX)/$(WXLUA_SRC_PATH)/wxwebview_bind.o : $(WXLUA_DIR)/$(WXLUA_SRC_PATH)/wxwebview_bind.cpp
	$(CPP) -c $< -o $@ $(CFLAGS) -DWXMAKINGDLL_BINDWXWEBVIEW=1

$(WEBVIEW_SO) : $(DESTPREFIX)/$(WXLUA_SRC_PATH)/wxwebview_bind.o final_executable
	(cd $(PWD)/$(DESTPREFIX)/$(PRODUCT_DIR); $(CPP) -shared -fPIC -o $(PWD)/$@ $(PWD)/$(DESTPREFIX)/wxlua/wxLua/modules/wxbind/src/wxwebview_bind.o $(CFLAGS) $(FINAL_EXECUTABLE) $(LD_EXTRA_FLAGS_WEBVIEW_SO) $(WX_LDFLAGS) -lwebkit2gtk-4.0 -ljavascriptcoregtk-4.0 $(LUAJIT_LDFLAGS))
endif

final_executable : $(DESTPREFIX)/$(EXECUTABLE)
ifeq ($(TARGET_PLATFORM),MSW)
	rm -rf $(DESTPREFIX)/$(PRODUCT_DIR)
	mkdir -p $(DESTPREFIX)/$(PRODUCT_DIR)
	cp $(DESTPREFIX)/$(EXECUTABLE) $(DESTPREFIX)/$(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
endif
ifeq ($(TARGET_PLATFORM),LINUX)
	rm -rf $(DESTPREFIX)/$(PRODUCT_DIR)
	mkdir -p $(DESTPREFIX)/$(PRODUCT_DIR)/lib
	cp $(DESTPREFIX)/$(EXECUTABLE) $(DESTPREFIX)/$(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
endif

$(DESTPREFIX)/$(PRODUCT) : final_executable $(WEBVIEW_SO)
ifeq ($(TARGET_PLATFORM),MSW)
ifneq ($(DEBUG),1)
	$(PATH_PREFIX)$(TOOL_PREFIX)strip $(DESTPREFIX)/$(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
endif
	cp $(LUAJIT_LIB) $(DESTPREFIX)/$(PRODUCT_DIR)
	cp -R ../wxSources/lib $(DESTPREFIX)/$(PRODUCT_DIR)
endif
ifeq ($(TARGET_PLATFORM),LINUX)
ifneq ($(DEBUG),1)
	$(PATH_PREFIX)$(TOOL_PREFIX)strip $(DESTPREFIX)/$(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
endif
	cp $(LUAJIT_LIB) $(DESTPREFIX)/$(PRODUCT_DIR)/lib
	cp $(WEBVIEW_SO) $(DESTPREFIX)/$(PRODUCT_DIR)/lib
	cp -R ../wxSources/lib $(DESTPREFIX)/$(PRODUCT_DIR)
endif

ifeq ($(TARGET_PLATFORM),MSW)
install: release

setup: build/release/$(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
	mkdir -p ../_latest_binaries
ifneq ($(CROSS),)
	($(WINE_PATH)/wine ../../Inno\ Setup\ 5/ISCC.exe ../wxSources/$(APPNAME).iss || exit 1)
else
	(/c/Program\ Files\ \(x86\)/Inno\ Setup\ 5/iscc ../wxSources/$(APPNAME).iss || exit 1)
endif
#mv Output/Setup_$(APPNAME).exe ../_latest_binaries
#(cd build/release && rm -rf $(MAKEDIR)/../_latest_binaries/$(APPNAME)Win* && mv $(PRODUCT_DIR) $(PRODUCT_DIR)Win && zip -r $(MAKEDIR)/../_latest_binaries/$(APPNAME)Win.zip $(PRODUCT_DIR)Win -x \*.DS_Store \*.svn* && mv $(PRODUCT_DIR)Win $(PRODUCT_DIR))
endif

ifeq ($(TARGET_PLATFORM),LINUX)
install: release

setup: $(DESTPREFIX)/$(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
	mkdir -p ../_latest_binaries
	rm -rf $(DESTPREFIX)/$(PRODUCT_DIR)/$(PRODUCT_DIR).AppDir
	../../AppImage/linuxdeploy/build-$(APPIMAGE_TOOL_ARCH)/bin/linuxdeploy -e $(DESTPREFIX)/$(PRODUCT_DIR)/$(FINAL_EXECUTABLE) -l $(LUAJIT_LIB) -d ../wxSources/LuaAppMaker.desktop -i ../wxlua/wxLua/art/wxlualogo.png --appdir $(DESTPREFIX)/$(PRODUCT_DIR)/$(PRODUCT_DIR).AppDir
	cp -R $(DESTPREFIX)/$(PRODUCT_DIR)/lib $(DESTPREFIX)/$(PRODUCT_DIR)/$(PRODUCT_DIR).AppDir/usr/bin
	rm -f $(DESTPREFIX)/$(PRODUCT_DIR)/$(PRODUCT_DIR).AppDir/usr/bin/lib/libluajit-*
	cp ../../AppImage/appimagetool-$(APPIMAGE_TOOL_ARCH).AppImage $(DESTPREFIX)/$(PRODUCT_DIR)/$(PRODUCT_DIR).AppDir/usr/bin
	../../AppImage/appimagetool-$(APPIMAGE_TOOL_ARCH).AppImage $(DESTPREFIX)/$(PRODUCT_DIR)/$(PRODUCT_DIR).AppDir
	mv -f $(PRODUCT_DIR)-$(APPIMAGE_PRODUCT_ARCH).AppImage ../_latest_binaries
endif

clean:
	rm -rf $(DESTPREFIX)
