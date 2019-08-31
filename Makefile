#
#   Makefile for wxLuaApp
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
LUAJIT_DIR = $(PWD)/../LuaJIT-2.0.5u

#  wxWidgets base directory
#  (Depends on your installation)
WX_DIR = $(PWD)/../../wxWidgets-3.0.3

#  Application name
APPNAME = wxLuaApp

#  Target platform and cross compile flag
ifeq ($(TARGET_ARCH),x86_64)
CROSS=mingw64
else
CROSS=mingw32
endif

PATH_PREFIX=/usr/local/homebrew/bin/

#  Object files

#  wxLua
WXLUA_DEBUG_O = wxldebug.o wxlstack.o
WXLUA_DEBUGGER_O = wxldserv.o wxldtarg.o wxlsock.o wxluadebugger_bind.o
WXLUA_O = lbitlib.o wxlbind.o wxlcallb.o wxlconsole.o wxllua.o wxlobject.o wxlstate.o wxlua_bind.o
WXBIND_O = wxadv_bind.o wxadv_wxladv.o wxaui_bind.o wxbase_base.o wxbase_bind.o wxbase_config.o \
  wxbase_data.o wxbase_datetime.o wxbase_file.o wxcore_appframe.o wxcore_bind.o wxcore_clipdrag.o \
  wxcore_controls.o wxcore_core.o wxcore_defsutils.o wxcore_dialogs.o wxcore_event.o wxcore_gdi.o \
  wxcore_geometry.o wxcore_help.o wxcore_image.o wxcore_mdi.o wxcore_menutool.o wxcore_picker.o \
  wxcore_print.o wxcore_sizer.o wxcore_windows.o wxcore_wxlcore.o wxgl_bind.o wxhtml_bind.o \
  wxhtml_wxlhtml.o wxmedia_bind.o wxnet_bind.o wxpropgrid_bind.o wxrichtext_bind.o wxstc_bind.o \
  wxwebview_bind.o wxxml_bind.o wxxrc_bind.o
WXLUAAPP_O = wxLuaApp.o ConsoleFrame.o lua_addition.o
LUAGL_O = luagl_const.o luagl_util.o luagl.o luaglu.o
WXLUA_ALL_O = $(addprefix wxlua/wxLua/modules/wxlua/debug/,$(WXLUA_DEBUG_O)) \
  $(addprefix wxlua/wxLua/modules/wxlua/debugger/,$(WXLUA_DEBUGGER_O)) \
  $(addprefix wxlua/wxLua/modules/wxlua/,$(WXLUA_O)) \
  $(addprefix wxlua/wxLua/modules/wxbind/src/,$(WXBIND_O)) \
  $(addprefix wxSources/,$(WXLUAAPP_O))

OBJECTS = $(WXLUA_ALL_O)

SUBDIRS = wxlua/wxLua/modules/wxlua/debug wxlua/wxLua/modules/wxlua/debugger wxlua/wxLua/modules/wxbind/src wxSources

#  wx libraries
WXLIB_LIST = core,base,gl,adv

ifeq ($(TARGET_PLATFORM),MSW)
 CPP_EXTRA_FLAGS = -I$(LUAJIT_DIR)/src -I../wxlua/wxLua -I../wxlua/wxLua/modules -I../wxlua/wxLua/modules/wxbind/include -I../wxlua/wxLua/modules/wxbind/setup -DLUA_COMPAT_MODULE
 ifneq ($(CROSS),)
  ifeq ($(TARGET_ARCH),i686)
   BUILD_DIR = build-win32
   TOOL_PREFIX = i686-w64-mingw32-
   LIB_SUFFIX = -3.0-i686-w64-mingw32
   HOST_CC = "gcc -m32"
   LUAJIT_HOSTCC_FLAGS = "HOST_XCFLAGS=-I. -DLUAJIT_OS=LUAJIT_OS_WINDOWS"
  else
   ifeq ($(TARGET_ARCH),x86_64)
    BUILD_DIR = build-win
    TOOL_PREFIX = x86_64-w64-mingw32-
    LIB_SUFFIX = -3.0-x86_64-w64-mingw32
    HOST_CC = "gcc"
   endif
  endif
  WINE_PATH=/Applications/EasyWine.app/Contents/Resources/wine/bin
 else
  LIB_SUFFIX = -3.0
 endif
 WX_LIB_DIR = $(WX_DIR)/$(BUILD_DIR)/lib
 WX_ARCH_DIR = $(WX_LIB_DIR)/wx/include/$(TOOL_PREFIX)msw-unicode-static-3.0
 WX_CPPFLAGS = -isystem $(WX_ARCH_DIR) -isystem $(WX_DIR)/include -D_LARGEFILE_SOURCE=unknown -D__WXMSW__
 WX_LDFLAGS = -L$(WX_LIB_DIR) -Wl,--subsystem,windows -mwindows -lwx_mswu_gl$(LIB_SUFFIX) -lopengl32 -lglu32 -lwx_mswu$(LIB_SUFFIX) -lwxregexu$(LIB_SUFFIX) -lwxexpat$(LIB_SUFFIX) -lwxtiff$(LIB_SUFFIX) -lwxjpeg$(LIB_SUFFIX) -lwxpng$(LIB_SUFFIX) -lwxzlib$(LIB_SUFFIX) -lwxscintilla$(LIB_SUFFIX) -lrpcrt4 -loleaut32 -lole32 -luuid -lwinspool -lwinmm -lshell32 -lcomctl32 -lcomdlg32 -ladvapi32 -lwsock32 -lgdi32
 LD_EXTRA_FLAGS = -static-libgcc -static-libstdc++
 EXECUTABLE = _$(APPNAME).exe_
 FINAL_EXECUTABLE = $(APPNAME).exe
 EXE_SUFFIX = .exe
 PRODUCT_DIR = $(APPNAME)
 PRODUCT = $(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
 EXTRA_OBJECTS = $(MSW_OBJECTS)
 LUAJIT_LIB = build/lib/lua51.dll
 WIN_FOPEN_O = build/lib/win_fopen.o
endif

CPP = $(PATH_PREFIX)$(TOOL_PREFIX)g++
CC = $(PATH_PREFIX)$(TOOL_PREFIX)gcc
AR = $(PATH_PREFIX)$(TOOL_PREFIX)ar
RANLIB = $(PATH_PREFIX)$(TOOL_PREFIX)ranlib

ifeq ($(MAKECMDGOALS),debug)
 DEBUG = 1
endif

ifeq ($(BUILD_CONFIGURATION),Debug)
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
export RANLIB

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
	mkdir -p build/lib
	$(CC) -c $< -o $@ $(CFLAGS)
endif

$(LUAJIT_LIB) : $(WIN_FOPEN_O)
ifeq ($(TARGET_PLATFORM),MSW)
	make -C $(LUAJIT_DIR) HOST_CC=$(HOST_CC) CFLAGS="" LDFLAGS="" TARGET_SHLDFLAGS="$(PWD)/$(WIN_FOPEN_O)" TARGET_FLAGS="-static-libgcc" CROSS=$(PATH_PREFIX)$(TOOL_PREFIX) $(LUAJIT_HOSTCC_FLAGS) TARGET_SYS=Windows || exit 1
	mkdir -p build/lib/lua
	cp $(LUAJIT_DIR)/src/lua51.dll build/lib
	cp -R $(LUAJIT_DIR)/src/jit build/lib/lua
	make -C $(LUAJIT_DIR) HOST_CC=$(HOST_CC) CFLAGS="" LDFLAGS="" TARGET_FLAGS="-static-libgcc" CROSS=$(PATH_PREFIX)$(TOOL_PREFIX) TARGET_SYS=Windows clean
endif

ALL_OBJECTS = $(OBJECTS) $(EXTRA_OBJECTS) $(RESOURCE)
DESTOBJECTS = $(addprefix $(DESTPREFIX)/,$(ALL_OBJECTS))
$(DESTPREFIX)/$(EXECUTABLE) : $(LUAJIT_LIB) $(DESTPREFIX) $(DESTOBJECTS)
	sh ../wxSources/record_build_date.sh >$(DESTPREFIX)/buildInfo.c
	$(CC) -c $(DESTPREFIX)/buildInfo.c -o $(DESTPREFIX)/buildInfo.o $(CFLAGS)
	$(CPP) -o $@ $(DESTOBJECTS) $(DESTPREFIX)/buildInfo.o $(WIN_FOPEN_O) $(LUAJIT_LIB) $(CFLAGS) $(LDFLAGS)

$(DESTPREFIX)/$(PRODUCT) : $(DESTPREFIX)/$(EXECUTABLE)
ifeq ($(TARGET_PLATFORM),MSW)
	rm -rf $(DESTPREFIX)/$(PRODUCT_DIR)
	mkdir -p $(DESTPREFIX)/$(PRODUCT_DIR)
	cp $(DESTPREFIX)/$(EXECUTABLE) $(DESTPREFIX)/$(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
	cp $(LUAJIT_LIB) $(DESTPREFIX)/$(PRODUCT_DIR)
	cp -R ../wxSources/scripts $(DESTPREFIX)/$(PRODUCT_DIR)
endif

ifeq ($(TARGET_PLATFORM),MSW)
install: setup

setup: build/release/$(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
	mkdir -p ../latest_binaries
ifneq ($(CROSS),)
	($(WINE_PATH)/wine ../../Inno\ Setup\ 5/ISCC.exe $(APPNAME).iss || exit 1)
else
	(/c/Program\ Files\ \(x86\)/Inno\ Setup\ 5/iscc $(APPNAME).iss || exit 1)
endif
	mv Output/Setup_$(APPNAME).exe ../latest_binaries
	(cd build/release && rm -rf $(MAKEDIR)/../latest_binaries/$(APPNAME)Win* && mv $(PRODUCT_DIR) $(PRODUCT_DIR)Win && zip -r $(MAKEDIR)/../latest_binaries/$(APPNAME)Win.zip $(PRODUCT_DIR)Win -x \*.DS_Store \*.svn* && mv $(PRODUCT_DIR)Win $(PRODUCT_DIR))
endif

clean:
	rm -rf $(DESTPREFIX)
