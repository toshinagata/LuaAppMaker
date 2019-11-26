LUA = ../../build-xcode/build/bin/luajit

MODULES = wxadd
CPPS = $(addprefix generated/,$(addsuffix .cpp,$(MODULES)))

all: $(CPPS)

generated/%.cpp : %_rules.lua %_*.i %_override.hpp
	$(LUA) -e"rulesFilename=\"$<\"" ../../wxlua/wxLua/bindings/genwxbind.lua
