LUA = ../../build-xcode/build/bin/luajit

MODULES = wxgraphics
CPPS = $(addprefix generated/,$(addsuffix .cpp,$(MODULES)))

all: $(CPPS)

generated/%.cpp : %_rules.lua %.i %_override.hpp
	$(LUA) -e"rulesFilename=\"$<\"" ../../wxlua/wxLua/bindings/genwxbind.lua
