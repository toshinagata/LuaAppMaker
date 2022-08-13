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
  if LuaApp.debug then
    local runflag = LuaApp.debug.dontStopAfterFirstRun
    LuaApp.debug.dontStopAfterFirstRun = true  --  Temporarily disable initial pausing
    LuaApp.debug.start()
    LuaApp.debug.dontStopAfterFirstRun = runflag
  end
end

