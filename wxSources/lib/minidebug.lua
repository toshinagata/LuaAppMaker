local miniDebug = {}
local breakpoints = {}
local willStop = true
local isRunning = false
local hasInited = false
local basedir = ""
local logfile = nil
local stackLevel = nil
local overLevel = nil
local savedOverLevel = {}
local handleOverOrOut = nil  -- HANDLE_OVER, HANDLE_OUT, HANDLE_CFUNC
local HANDLE_OVER = 1
local HANDLE_OUT = 2
local HANDLE_CFUNC = 3
local baseLevel = 0    -- Usually zero, but if we are stopping at a function call and want to fake that we are just before the call, then this is 1

miniDebug.env = nil
miniDebug.verbose = nil      -- 1: show message, 2: 1 + debug.log

LuaApp.config.showConsole = true

local thisSource = debug.getinfo(1).source

local socket = nil --  The socket to connect with the server

local buffer = ""
local function Receive(nbytes)
  if socket and socket:Ok() and socket:IsConnected() then
    local buf, nsize
    if #buffer < nbytes then
      buf = socket:Read(nbytes - #buffer)
      nsize = socket:LastCount()
      buffer = buffer .. buf:sub(1, nsize)
    end
    if #buffer > nbytes then
      buf = buffer:sub(1, nbytes)
      buffer = buffer:sub(nbytes + 1, -1)
    else
      buf = buffer
      buffer = ""
    end
    if miniDebug.verbose and #buf > 0 then
      print("(In)  " .. buf)
    end
    return buf
  end
  return nil
end

local function Send(msg)
  if socket and socket:Ok() and socket:IsConnected() then
    if miniDebug.verbose then
      print("(Out) " .. msg)
    end
    socket:Write(msg)
    local n = socket:LastCount()
    while n < #msg do
      msg = msg:sub(n + 1, -1)
      if not socket:Wait(10) then
        print("(Out)[Send timeout, the following chars are not sent: "..msg.."]")
        return
      end
      socket:Write(msg)
      n = socket:LastCount()
    end
  end
end

local function Output(msg)
  if not logfile then
    logfile = io.open(basedir .. "debug.log", "w")
  end
  logfile:write(msg)
  logfile:flush()
end

local function RemoveBaseDir(str)
  if str:sub(1, 1) == "@" then
    str = str:sub(2, -1)
  end
  if basedir and str:sub(1, #basedir) == basedir then
    str = str:sub(#basedir + 1, -1)
  end
  return str
end

local function ToString(val, options)
  local t = type(val)
  if t == "number" or t == "nil" then
    return tostring(val)
  elseif t == "string" then
    return string.format("%q", val)
  elseif t == "userdata" then
    local s = tostring(val)
    local s1 = s:match("userdata: 0x[0-9a-f]+ (%[wx.*)")
    if s1 then s = s1 end
    return string.format("%q", s)
  elseif t == "function" then
    local info = debug.getinfo(val)
    local s
    if info then
      s = string.format("function %sat %s:%d", (info.name and info.name.." ") or "", RemoveBaseDir(info.source), info.linedefined)
    else
      s = tostring(val)
    end
    return string.format("%q", s)
  elseif t == "table" then
    options = options or { loop = {} }
    local loop = options.loop
    local limit = options.limit or 10
    for i = 1, #loop do
      if loop[i] == val then
        --  Avoid circular reference
        return string.format("%q", "<Circular reference> "..tostring(val))
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
        if k:find("[^_%w]") then
          k = string.format("[%q]", k)
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
    return string.format("%q", tostring(val))
  end
end

local function GetUpvaluesAndLocals(level, func)
  if not func then return nil, nil, nil end
  local locals = {}
  local varargs = {}
  local ups = {}
  --  Upvalues
  local i = 1
  while true do
    local name, value = debug.getupvalue(func, i)
    if not name then break end
    ups[name] = value
    i = i + 1
  end
  --  Local variables
  i = 1
  while true do
    local name, value
    name, value = debug.getlocal(level, i)
    if not name then break end
    if name ~= "(*temporary)" then
      locals[name] = value
    end
    i = i + 1
  end
  --  Var args
  i = 1
  while true do
    local name, value
    name, value = debug.getlocal(level, -i)
    if not name or name ~= "(*vararg)" then break end
    varargs[i] = value
    i = i + 1
  end
  return ups, locals, varargs
end

local function CaptureVars(level)
  local func = (debug.getinfo(level, "f") or {}).func
  local ups, locals, varargs = GetUpvaluesAndLocals(level + 1, func)
  if not ups then return {} end
  local vars = ups
  --  Local variables
  for k, v in pairs(locals) do
    if string.sub(k, 1, 1) ~= '(' then vars[k] = v end
  end
  --  Var args
  vars['...'] = {}
  for i, v in ipairs(varargs) do
    vars['...'][i] = v
  end
  --  The globals are accessed though metatable methods
  local env = getfenv(func)
  setmetatable(vars, { __index = env, __newindex = env })
  return vars
end

local function StackLevel(level)
  local i = level
  while true do
    i = i + 1
    local info = debug.getinfo(i, "l")
    if not info then break end
  end
  return i - level
end

local function ProcessLines(event)
  while true do
    local buf
    while true do
      buf = Receive(1024)
      if buf and #buf > 0 then break end
      if isRunning then return end  -- Continue running
    end
    local buf2 = ""
    local s, e = buf:find("\n")
    if s then
      buf2 = buf:sub(e + 1, -1)
      buf = buf:sub(1, s - 1)
    end
    local com = buf:match("^%w+")
    local init = #com + 1
    if buf:byte(-1) == 10 then buf = buf:sub(1, -2) end
    if com == "SUSPEND" then
      if isRunning then
        willStop = true
      end
    elseif com == "BASEDIR" then
      local dir = buf:match("%s*(%S+)", init)
      basedir = dir
      Send("200 OK\n")
    elseif com == "SETB" then
      local file, line = buf:match("%s*(.-)%s+(%d+)", init)
      if file and line then
        line = tonumber(line)
        file = "@" .. basedir .. file
        if not breakpoints[line] then
          breakpoints[line] = {}
        end
        breakpoints[line][file] = true
      end
      Send("200 OK\n")
    elseif com == "DELB" then
      local file, line = buf:match("%s*(.-)%s+(%d+)", init)
      if file and line then
        line = tonumber(line)
        if file == "*" then
          breakpoints[line] = nil
        else
          file = "@" .. basedir .. file
          local b = breakpoints[line]
          if b then
            b[file] = nil
          end
        end
      end
      Send("200 OK\n")
    elseif com == "LOAD" then
      local count, file = buf:match("%s*(%d+)%s+(.*)", init)
      count = tonumber(count)
      while count > #buf2 do
        buf2 = buf2 .. Receive(count - #buf2)
      end
      --  We will ignore the content for now
      Send("200 OK\n")
    elseif com == "RUN" then
      Send("200 OK\n")
      willStop = false  --  Continue to next breakpoint
      break  -- Continue execution
    elseif com == "SUSPEND" then
      Send("200 OK\n")
      willStop = true   --  Stop at next hook
      break  -- Continue execution
    elseif com == "STEP" then
      Send("200 OK\n")
      willStop = true   --  Stop at next hook
      break
    elseif com == "OVER" or com == "OUT" then
      Send("200 OK\n")
      if not isRunning then
        if com == "OVER" then
          handleOverOrOut = HANDLE_OVER
        else
          handleOverOrOut = HANDLE_OUT
        end
        willStop = true
      end
      break  --  Continue execution
    elseif com == "DONE" then
      debug.sethook(nil)  --  Stop debugging
      socket:Close()      --  Stop communicating with the server
      break
    elseif com == "EXEC" then
      local val, err
      local expr = buf:match("(.-)%-%-", init)
      val, err = loadstring(expr)
      if val then
        local env = CaptureVars(4 + baseLevel)  --  1: CaptureVars, 2: ProcessLines, 3: Hook, 4: active function
        setfenv(val, env)
        val, err = pcall(val)
        if val then
          val = err
          err = nil
        else
          err = "pcall error: " .. err
          val = nil
        end
      else
        err = "loadstring error: " .. expr
      end
      if err then
        Send(string.format("401 ERROR %d\n", #err + 1))
        Send(err .. "\n")
      else
        local s = ToString(val)
        local res = string.format("return {%q}", s)
        Send(string.format("200 OK %d\n", #res + 1))
        Send(res .. "\n")
      end
    elseif com == "STACK" then
      local val = {}
      for i = baseLevel + 3, baseLevel + 5 do
        local info = debug.getinfo(i)
        if not info then break end
        local ups, locals, varargs = GetUpvaluesAndLocals(i + 1, info.func)
        if #varargs > 0 then
          locals['...'] = varargs
        end
        for k, v in pairs(ups) do
          ups[k] = {v, select(2, pcall(tostring, v))}
        end
        for k, v in pairs(locals) do
          locals[k] = {v, select(2, pcall(tostring, v))}
        end
        table.insert(val, {{info.name, info.source, info.linedefined, info.currentline, info.what, info.namewhat, info.short_src}, locals, ups})
      end
      local res = "return "..ToString(val)
      Send("200 OK " .. res .. "\n")
      if miniDebug.reportStackValue then
        Output("Responce to STACK: "..res.."\n")
      end
    else
      Send("402 ERROR\n")
    end
    --if isRunning then
      -- Called from OnSocketConnection: Process only one command
      --break
    --end 
  end
end

local function OnSocketConnection(event)
  print("OnSocketConnection invoked")
  local socketEvent = event:GetSocketEvent()
  if socketEvent == wx.wxSOCKET_CONNECTION then
    if socket:Ok() and socket:IsConnected() then
      print("Connection established.")
    else
      print("SOCKET_CONNECTION event is caught but failed to connect.")
    end
  elseif socketEvent == wx.wxSOCKET_LOST then
    print("Connection lost.")
  elseif socketEvent == wx.wxSOCKET_INPUT then
    ProcessLines()
  end
end

local overSeen = false
local lastCountTime

local function Hook(event, line)
  local info
  if miniDebug.reportStack and (miniDebug.reportStack ~= true or event == "call" or event == "return") then
    local msg = ""
    local i, j, info2
    i = 2
    j = StackLevel(2)
    msg = string.format("event = %s, overLevel = %s, savedOverLevel = %s\n", event,  tostring(overLevel), ToString(savedOverLevel))
    while true do
      info2 = debug.getinfo(i)
      if info2 then
        msg = msg .. string.format("Stack %d: name = %s, what = %s, currentline = %d, source = %s\n", j + 2 - i, info2.name, info2.what, info2.currentline, info2.source)
      else
        break
      end
      i = i + 1
    end
    Output(msg)
  end
  if miniDebug.verbose then
    print("HOOK", event, line)
    if event == "call" or event == "return" then
      local info1 = debug.getinfo(2)
      if info1.what ~= "C" then
        print(info1.linedefined, info1.lastlinedefined, info1.source, info1.name, info1.what, info1.currentline)
        print("stack level = "..tostring(StackLevel(2)))
      end
    end
  end
  if not willStop then
    if overLevel and (overLevel < 0 or event == "call" or event == "return") then
      --  Handle OVER/OUT
      info = debug.getinfo(2)
      local level = StackLevel(2)
      local level1 = (overLevel < 0 and (-overLevel - 1)) or overLevel
      local discardOverLevel = function (level0)
        while level0 <= level1 do
          overLevel = table.remove(savedOverLevel)
          if not overLevel then break end
          level1 = (overLevel < 0 and (-overLevel - 1)) or overLevel
        end
      end
      if event == "call" then
        if level < level1 then
          discardOverLevel(level)
          if overLevel then
            table.insert(savedOverLevel, overLevel)
            overLevel = (info.what == "C" and (-level - 1)) or level
          else
            goto stopHere
          end
        else
          if level > level1 then
            --  Save current overLevel and enter a new 'OVER' loop
            table.insert(savedOverLevel, overLevel)
          end
          --  Replace the current overLevel
          overLevel = (info.what == "C" and (-level - 1)) or level
        end
      elseif overLevel >= 0 then
        -- "Lua OVER" mode
        if event == "return" then
          discardOverLevel(level)
          if not overLevel then
            willStop = true  --  Stop at the next hook
          end
        end
      else
        -- "C OVER" mode
        if level < level1 then
          discardOverLevel(level + 1)
          if not overLevel then
            goto stopHere
          end
        end
      end
    end
    if event == "line" then
      if not breakpoints[line] then return end
      info = debug.getinfo(2)
      if not breakpoints[line][info.source] then return end
      goto stopHere
    end
    if event == "count" then
      local countTime = wx.wxGetLocalTimeMillis():ToDouble()
      if not lastCountTime then lastCountTime = countTime end
      if countTime - lastCountTime > 50 then
        --  Handle every 50 milliseconds
        lastCountTime = countTime
        ProcessLines(event)
        return  --  Continue running
      end
    end
    return  --  Don't stop here
  end
  ::handleOver::
  if handleOverOrOut then
    willStop = false
    if event == "call" then
      -- OVER, call: record current stack level
      -- OUT, call: record current+1 stack level (i.e. the stack level before 'call')
      local n = (handleOverOrOut == HANDLE_OVER and 2) or 3
      if not info then info = debug.getinfo(n) end
      overLevel = StackLevel(n)
      savedOverLevel = {}
      if info.what == "C" then
        --  'C OVER' mode
        overLevel = -overLevel - 1
      end
      handleOverOrOut = nil
      return  --  Continue execution
    elseif handleOverOrOut == HANDLE_OUT then
      --  OUT, non-call: record current stack level and enter 'Lua OVER' mode
      overLevel = StackLevel(2)
      savedOverLevel = {}
      handleOverOrOut = nil
      return
    else
      goto stopHere  --  Stop here
    end
  end
  ::stopHere::
  if not info then
    info = debug.getinfo(baseLevel + 2)
    if info.what == "C" then return end  --  Don't stop inside a C function
    if info.source == thisSource then return end  --  Don't stop inside this file
  end
  --  Discard 'OVER' loop
  overLevel = nil
  savedOverLevel = {}
  handleOverOrOut = nil
  if hasInited then
    local source = info.source
    if source:sub(1, 1) == "@" then
      --  Remove @ and basedir
      source = source:sub(2, -1)
      if source:sub(1, #basedir) == basedir then
        source = source:sub(#basedir + 1, -1)
      end
    end
    if miniDebug.reportPause then
      Output(string.format("Paused at %d in %s\n", info.currentline, source))
    end
    Send(string.format("202 Paused %s %d\n", source, info.currentline))
  end
  isRunning = false
  ProcessLines(event)
  isRunning = true
  if not hasInited then
    hasInited = true
  end
  if baseLevel == 1 then
    --  We are faking as if we are 'before' a function call, but in reality we are already
    --  in a function call. In this case, 'call' event is handled again and execution is continued
    if miniDebug.verbose then
      Output(string.format("We are at %d in %s\n", info.currentline, info.source))
    end
    baseLevel = 0
    goto handleOver
  end
end

local function StartClient(port)
  if not socket then
    local addr = wx.wxIPV4address()
    assert(addr:LocalHost())
    assert(addr:Service(port or 8172))
    socket = wx.wxSocketClient(wx.wxSOCKET_NOWAIT)
    if miniDebug.verbose then print("socket = " .. tostring(socket)) end
    --socket:SetNotify(wx.wxSOCKET_INPUT_FLAG + wx.wxSOCKET_CONNECTION_FLAG + wx.wxSOCKET_LOST_FLAG)
    --socket:Notify(true)
    --socket:SetEventHandler(LuaApp)
    --LuaApp:Connect(-1, wx.wxEVT_SOCKET, OnSocketConnection)
    socket:Connect(addr, false)
    while not socket:Wait(5, 0) do
      if miniDebug.verbose then print(string.format("Waiting for connection to %s (%s):%d...", addr:Hostname(), addr:IPAddress(), addr:Service())) end
    end
    hasInited = false
    ProcessLines()  --  Process startup commands
    hasInited = true
    willStop = true --  Stop immediately after "RUN"
    stackLevel = nil
    stopLevel = nil
    savedStopLevel = {}
  end
end

local function start()
  if hasInited then return end
  LuaApp.config.showConsole = true
  StartClient()
  debug.sethook(Hook, "crl", 10000)
end

miniDebug.start = start

return miniDebug
