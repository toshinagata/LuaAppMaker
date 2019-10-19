-- ----------------------------------------------------------------------------
-- Rules to build the wxWidgets binding for wxLua
--  load using : $lua -e"rulesFilename=\"rules.lua\"" genwxbind.lua
-- ----------------------------------------------------------------------------

-- ----------------------------------------------------------------------------
-- Set the root directory of the wxLua distribution, used only in this file
wxlua_dir = "../../wxlua/wxLua/"

-- ============================================================================
-- Set the Lua namespace (Lua table) that the bindings will be placed into.
--   See wxLuaBinding::GetLuaNamespace(); eg. wx.wxWindow(...)
hook_lua_namespace = "wx"

-- Set the unique C++ "namespace" for the bindings, not a real namespace, but
--   a string used in declared C++ objects to prevent duplicate names.
--   See wxLuaBinding::GetBindingName().
hook_cpp_namespace = "wxgraphics"

-- ============================================================================
-- Set the directory to output the bindings to, both C++ header and source files
output_cpp_header_filepath = "generated"
output_cpp_filepath        = "generated"

-- ============================================================================
-- Set the DLLIMPEXP macros for compiling these bindings into a DLL
--  Use "WXLUA_NO_DLLIMPEXP" and "WXLUA_NO_DLLIMPEXP_DATA" for no IMPEXP macros

output_cpp_impexpsymbol     = "WXDLLIMPEXP_BINDWXCORE"
output_cpp_impexpdatasymbol = "WXDLLIMPEXP_DATA_BINDWXCORE"

-- ----------------------------------------------------------------------------
-- Set the name of the header file that will have the #includes from the
--   bindings in it. This will be used as #include "hook_cpp_header_filename" in
--   the C++ wrapper files, so it must include the proper #include path.
--hook_cpp_header_filename = "wxbind/include/"..hook_cpp_namespace.."_bind.h"
hook_cpp_header_filename = hook_cpp_namespace.."_bind.h"

-- ----------------------------------------------------------------------------
-- Set the name of the main binding file that will have the glue code for the
--   bindings in it. This file along with the output from the *.i files will be
--   placed in the "output_cpp_filepath".
hook_cpp_binding_filename = hook_cpp_namespace.."_bind.cpp"

-- ----------------------------------------------------------------------------
-- Generate only a single output C++ binding source file with the name of
--   hook_cpp_binding_filename, as opposed to generating a single cpp file
--   for each *.i file plus the hook_cpp_binding_filename file.
output_single_cpp_binding_file = false

-- ----------------------------------------------------------------------------
-- Set the name of the subclassed wxLuaBinding class
hook_cpp_binding_classname = "wxLuaBinding_"..hook_cpp_namespace

-- ----------------------------------------------------------------------------
-- Set the function names that wrap the output structs of defined values,
--   objects, events, functions, and classes.
hook_cpp_define_funcname   = "wxLuaGetDefineList_"..hook_cpp_namespace
hook_cpp_string_funcname   = "wxLuaGetStringList_"..hook_cpp_namespace
hook_cpp_object_funcname   = "wxLuaGetObjectList_"..hook_cpp_namespace
hook_cpp_event_funcname    = "wxLuaGetEventList_"..hook_cpp_namespace
hook_cpp_function_funcname = "wxLuaGetFunctionList_"..hook_cpp_namespace
hook_cpp_class_funcname    = "wxLuaGetClassList_"..hook_cpp_namespace

-- ----------------------------------------------------------------------------
-- Set any #includes or other C++ code to be placed verbatim at the top of
--   every generated cpp file or "" for none
hook_cpp_binding_includes =
[[
#define wxBLACK_INSTANCE (*wxBLACK)
]]

-- ----------------------------------------------------------------------------
-- Set any #includes or other C++ code to be placed verbatim below the
--   #includes of every generated cpp file or "" for none
--   X.h defines Above and Below as numbers, undef them for wx/layout.h
hook_cpp_binding_post_includes =
"#ifdef Above\n    #undef Above\n#endif\n"..
"#ifdef Below\n    #undef Below\n#endif\n"

-- ----------------------------------------------------------------------------
-- Add additional include information or C++ code for the binding header file,
--  hook_cpp_header_filename.
--  This code will be place directly after any #includes at the top of the file
hook_cpp_binding_header_includes =
    "#include \"wxbind/include/wxbinddefs.h\"\n"..
    "#include \"wxluasetup.h\"\n"..
    "#include \"wxbind/include/wxbase_bind.h\"\n"..
    "#include \"wxbind/include/wxcore_bind.h\"\n"

-- ----------------------------------------------------------------------------
-- Set any #includes or other C++ code to be placed verbatim at the top of
--   the single hook_cpp_binding_filename generated cpp file or "" for none
hook_cpp_binding_source_includes = ""
--[[
int LUACALL wxlua_pushwxArrayDoubletable(lua_State *L, const wxArrayDouble &doubleArray)
{
    size_t idx, count = doubleArray.GetCount();
    lua_createtable(L, count, 0);

    for (idx = 0; idx < count; ++idx)
    {
        lua_pushnumber(L, doubleArray[idx]);
        lua_rawseti(L, -2, idx + 1);
    }
    return idx;
}
]]

--[=[
[[
// wxWidgets >= 2.7 doesn't have static versions of stock GDI objects anymore
wxColour* wxLua_wxBLACK = NULL;
wxColour* wxLua_wxWHITE = NULL;
wxColour* wxLua_wxRED = NULL;
wxColour* wxLua_wxBLUE = NULL;
wxColour* wxLua_wxGREEN = NULL;
wxColour* wxLua_wxCYAN = NULL;
wxColour* wxLua_wxLIGHT_GREY = NULL;
wxPen* wxLua_wxRED_PEN = NULL;
wxPen* wxLua_wxCYAN_PEN = NULL;
wxPen* wxLua_wxGREEN_PEN = NULL;
wxPen* wxLua_wxBLACK_PEN = NULL;
wxPen* wxLua_wxWHITE_PEN = NULL;
wxPen* wxLua_wxTRANSPARENT_PEN = NULL;
wxPen* wxLua_wxBLACK_DASHED_PEN = NULL;
wxPen* wxLua_wxGREY_PEN = NULL;
wxPen* wxLua_wxMEDIUM_GREY_PEN = NULL;
wxPen* wxLua_wxLIGHT_GREY_PEN = NULL;
wxBrush* wxLua_wxBLUE_BRUSH = NULL;
wxBrush* wxLua_wxGREEN_BRUSH = NULL;
wxBrush* wxLua_wxWHITE_BRUSH = NULL;
wxBrush* wxLua_wxBLACK_BRUSH = NULL;
wxBrush* wxLua_wxGREY_BRUSH = NULL;
wxBrush* wxLua_wxMEDIUM_GREY_BRUSH = NULL;
wxBrush* wxLua_wxLIGHT_GREY_BRUSH = NULL;
wxBrush* wxLua_wxTRANSPARENT_BRUSH = NULL;
wxBrush* wxLua_wxCYAN_BRUSH = NULL;
wxBrush* wxLua_wxRED_BRUSH = NULL;
wxFont* wxLua_wxNORMAL_FONT = NULL;
wxFont* wxLua_wxSMALL_FONT = NULL;
wxFont* wxLua_wxITALIC_FONT = NULL;
wxFont* wxLua_wxSWISS_FONT = NULL;
wxCursor* wxLua_wxSTANDARD_CURSOR = NULL;
wxCursor* wxLua_wxHOURGLASS_CURSOR = NULL;
wxCursor* wxLua_wxCROSS_CURSOR = NULL;

]]
--]=]

-- ============================================================================
-- Set the bindings directory that contains the *.i interface files
interface_filepath = "."

-- ----------------------------------------------------------------------------
-- A list of interface files to use to make the bindings. These files will be
--   converted into *.cpp and placed in the output_cpp_filepath directory.
--   The files are loaded from the interface_filepath.
interface_fileTable =
{
    "wxgraphics.i"
}

-- ----------------------------------------------------------------------------
-- A list of files that contain bindings that need to be overridden or empty
--   table {} for none.
--   The files are loaded from the interface_filepath.
override_fileTable = { "wxgraphics_override.hpp" }

-- ============================================================================
-- A table containing filenames of XXX_datatype.lua from other wrappers to
--  to define classes and data types used in this wrapper
--  NOTE: for the base wxWidgets wrappers we don't load the cache since they
--        don't depend on other wrappers and can cause problems when interface
--        files are updated. Make sure you delete or have updated any cache file
--        that changes any data types used by this binding.

datatype_cache_input_fileTable = { wxlua_dir.."bindings/wxwidgets/wx_datatypes.lua" }

-- ----------------------------------------------------------------------------
-- The file to output the data type cache for later use with a binding that
--   makes use of data types (classes, enums, etc) that are declared in this
--   binding. The file will be generated in the interface_filepath.

datatypes_cache_output_filename = "generated/"..hook_cpp_namespace.."_datatypes.lua"

-- ============================================================================
-- Declare functions or member variables for the derived wxLuaBinding class
--   that will be generated for this binding. The string will be copied verbatim
--   into the body of the hook_cpp_binding_classname class declaration in the
--   hook_cpp_header_filename header file. May be remmed out to ignore it.
-- See usage in the wxWidgets wxbase_rules.lua file.

wxLuaBinding_class_declaration =
[[
    virtual bool RegisterBinding(const wxLuaState& wxlState);
]]

-- ----------------------------------------------------------------------------
-- Implement the functions or member variables for the derived wxLuaBinding
--   class that you have declared. The string will be copied into the
--   hook_cpp_binding_filename source file. May be remmed out to ignore it.
-- See usage in the wxWidgets wxbase_rules.lua file.

wxLuaBinding_class_implementation =
"bool "..hook_cpp_binding_classname.."::RegisterBinding(const wxLuaState& wxlState)\n"..
[[
{
    bool ret = wxLuaBinding::RegisterBinding(wxlState);
    return ret;
}
]]

-- ============================================================================
-- Add additional conditions here
-- example: conditions["DOXYGEN_INCLUDE"] = "defined(DOXYGEN_INCLUDE)"

-- ----------------------------------------------------------------------------
-- Add additional data types here
--AllocDataType("wxLuaObject", "class", false)

-- ============================================================================
-- Generate comments into binding C++ code
comment_cpp_binding_code = true
