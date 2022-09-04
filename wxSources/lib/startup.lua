function LuaApp.LoadSettings()
  local fname = LuaApp.settingsPath
  if wx.wxFileName.FileExists(fname) then
    dofile(fname)  --  The settings are recorded as "c.xxxx = value"
  end
end

function LuaApp.SaveSettings()
  local fname = LuaApp.settingsPath
  local f = io.open(fname, "wt")
  f:write("local c = LuaApp.config\n")
  for key, value in pairs(LuaApp.config) do
    local valuestr
    if type(value) == "string" then
      valuestr = string.format("%q", value)
    elseif type(value) == "number" then
      valuestr = tostring(value)
    end
    if valuestr then
      f:write(string.format("c.%s = %s\n", key, valuestr))
    end
  end
  f:close()
end

--  Wrapper function, in case miniDebug.start() should be called from inside C function
function LuaApp.startDebug()
  if LuaApp.debug and not LuaApp.debug.hasStarted then
    local runflag = LuaApp.debug.dontStopAfterFirstRun
    LuaApp.debug.dontStopAfterFirstRun = true  --  Temporarily disable initial pausing
    LuaApp.debug.start()
    LuaApp.debug.dontStopAfterFirstRun = runflag
  end
end

--  Utility functions

function string.quote(str)
  str = string.format("%q", str):gsub("\\\n", "\\n")
  return str
end

function ToString(val, options)
  local t = type(val)
  if t == "number" or t == "nil" then
    return tostring(val)
  elseif t == "string" then
    return string.quote(val)
  elseif t == "userdata" then
    local s = tostring(val)
    local s1 = s:match("userdata: 0x[0-9a-f]+ (%[wx.*)")
    if s1 then
      s = s1
      local classname = s:match("%[(wx%w+)%(")
      if classname == "wxPoint" or classname == "wxPoint2DDouble" then
        s = "["..classname.."("..tostring(val.X)..","..tostring(val.Y)..")]"
      elseif classname == "wxSize" or classname == "wxSize2DDouble" then
        s = "["..classname.."("..tostring(val.Width)..","..tostring(val.Height)..")]"
      elseif classname == "wxRect" or classname == "wxRect2DDouble" then
        s = "["..classname.."("..tostring(val.X)..","..tostring(val.Y)..","..tostring(val.Width)..","..tostring(val.Height)..")]"
      end
    end
    return string.quote(s)
  elseif t == "function" then
    local info = debug.getinfo(val)
    local s
    if info then
      local source = info.source
      if RemoveBaseDir then
        source = RemoveBaseDir(source)
      end
      s = string.format("function %sat %s:%d", (info.name and info.name.." ") or "", source, info.linedefined)
    else
      s = tostring(val)
    end
    return string.quote(s)
  elseif t == "table" then
    options = options or { loop = {} }
    local loop = options.loop
    local limit = options.limit or 10
    for i = 1, #loop do
      if loop[i] == val then
        --  Avoid circular reference
        return string.quote("<Circular reference> "..tostring(val))
      end
    end
    table.insert(loop, val)
    local s = "{"
    local len = #val
    local n1 = 0
    for i = 1, len do
      if n1 >= limit then break end
      s = s .. ToString(val[i], options) .. ","
      n1 = n1 + 1
    end
    n1 = 0
    for k, v in pairs(val) do
      if type(k) == "number" and math.floor(k) == k and k >= 1 and k <= len then
        --  Skip
      else
        k = tostring(k)
        if k == "" or k:find("[^_A-Za-z]") then
          k = "["..string.quote(k).."]"
        end
        s = s .. k .. "=" .. ToString(v, options) .. ","
      end
    end
    if s:sub(-1) == "," then
      s = s:sub(1, -2)
    end
    s = s .. "}"
    table.remove(loop)
    return s
  else
    return string.quote(tostring(val))
  end
end

function table.isempty(t)
  for k,v in pairs(t) do
    return false
  end
  return true
end

function table.countkeys(t)
  local c = 0
  for k,v in pairs(t) do
    c = c + 1
  end
  return c
end
