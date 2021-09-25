local function ChooseDir(event, msg, textctrl, text2, text3)
  local platform = string.match(wx.wxGetOsDescription(), "^(%S*)")
  local fname = wx.wxDirSelector(msg, "", wx.wxDD_DIR_MUST_EXIST)
  if fname ~= "" then
    textctrl:SetValue(fname)
    -- if text2 is empty, then set the folder name
    local fn = wx.wxFileName(fname)
    local appname = fn:GetName()
    if text2 and text2:GetValue() == "" then
      text2:SetValue(appname)
    end
    -- if text3 is empty, then look for "appname" + extension
    if text3 and text3:GetValue() == "" then
      fn:AppendDir(appname)
      fn:SetName(appname)
      if platform == "Windows" then
        fn:SetExt("ico")
      elseif platform == "Mac" or platform == "macOS" then
        fn:SetExt("icns")
      else
        fn:SetExt("png")
      end
      if fn:FileExists() then
        text3:SetValue(fn:GetFullPath())
      end
    end
  end
end

local function ChooseFile(event, msg, textctrl)
  local platform = string.match(wx.wxGetOsDescription(), "^(%S*)")
  local extension, description
  if platform == "Windows" then
    description = "Windows ICON File"
    extension = "ico"
  elseif platform == "Mac" then
    description = "Mac Icon File"
    extension = "icns"
  end
  if description then
    description = description .. "(*." .. extension .. ")|*." .. extension .. "|"
  else
    description = ""
    extension = ""
  end
  description = description .. "All files(*.*)|*.*"
  local fname = wx.wxFileSelector(msg, "", "", extension, description, wx.wxFD_OPEN + wx.wxFD_FILE_MUST_EXIST)
  if fname ~= "" then
    textctrl:SetValue(fname)
  end
end

function LuaApp.CreateApp()
  local dialog = wx.wxDialog(wx.NULL, -1, "Create Standalone Application", wx.wxDefaultPosition, wx.wxDefaultSize, wx.wxCAPTION + wx.wxSYSTEM_MENU)  --  No close box
  -- ["SpinFlow data folder", "choose" button]
  -- textctrl
  -- "Import the latest data on startup"
  -- Standard buttons
  local panel1 = wx.wxPanel(dialog, -1, wx.wxDefaultPosition, wx.wxSize(400, 200))

  local text11 = wx.wxStaticText(panel1, -1, "wxLua Script Folder", wx.wxPoint(6, 6))
  local button1 = wx.wxButton(panel1, -1, "Choose...", wx.wxPoint(320, 6), wx.wxSize(74, 18))
  local text12 = wx.wxTextCtrl(panel1, 1, "", wx.wxPoint(6, 28), wx.wxSize(388, 40), wx.wxTE_READONLY)

  local text21 = wx.wxStaticText(panel1, -1, "Application Name", wx.wxPoint(6, 72))
  local text22 = wx.wxTextCtrl(panel1, 1, "", wx.wxPoint(6, 94), wx.wxSize(388, 20))

  local text31 = wx.wxStaticText(panel1, -1, "Application Icon", wx.wxPoint(6, 124))
  local button3 = wx.wxButton(panel1, -1, "Choose...", wx.wxPoint(320, 124), wx.wxSize(74, 18))
  local text32 = wx.wxTextCtrl(panel1, 1, "", wx.wxPoint(6, 146), wx.wxSize(388, 40), wx.wxTE_READONLY)

  button1:Connect(-1, wx.wxEVT_COMMAND_BUTTON_CLICKED,
    function (event) ChooseDir(event, "Choose wxLua script folder", text12, text22, text32) end)
  button3:Connect(-1, wx.wxEVT_COMMAND_BUTTON_CLICKED,
    function (event) ChooseFile(event, "Choose Application Icon file", text32) end)

  local sizerb = dialog:CreateButtonSizer(wx.wxOK + wx.wxCANCEL)
  local sizer1 = wx.wxBoxSizer(wx.wxVERTICAL)

  sizer1:Add(panel1, wx.wxSizerFlags(1))
  sizer1:Add(sizerb, wx.wxSizerFlags(0):Border(wx.wxALL, 5))
  dialog:SetSizerAndFit(sizer1)

  local script_folder, app_name, icon_path
  while true do
    if (dialog:ShowModal() == wx.wxID_OK) then
      app_name = text22:GetValue()
      if app_name and (string.find(app_name, "/", 1, true) or string.find(app_name, "\\", 1, true)) then
        --  Path separator character is present
        wx.wxMessageBox("Please avoid '/' or '\\' in the application name.", "Name Error")
        --  Do dialog again
      else
        script_folder = text12:GetValue()
        icon_path = text32:GetValue()
        break  --  OK
      end
    else
      break  --  Canceled
    end
  end
  dialog:Destroy()
  return script_folder, app_name, icon_path
end

function LuaApp.CreateISS(appname)
  local script = [[
#define MyAppName "@@1@@"
#define Version ""

[Setup]
AppName = {#MyAppName}
AppVerName = {#MyAppName} {#Version}
DefaultDirName = {pf}\{#MyAppName}
DefaultGroupName = {#MyAppName}
UninstallDisplayIcon = {app}\{#MyAppName}.exe
OutputBaseFileName = Setup_{#MyAppName}
@@2@@ArchitecturesInstallIn64BitMode=x64
@@2@@ArchitecturesAllowed=x64

[Files]
Source: "{#MyAppName}.exe"; DestDir: {app}
Source: "lua51.dll"; DestDir: {app}
Source: "scripts\*"; DestDir: {app}\scripts; Flags: recursesubdirs
Source: "lib\*"; DestDir: {app}\lib; Flags: recursesubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppName}.exe"
Name: "{group}\Uninstall {#MyAppName}"; Filename: {uninstallexe}
]]
  script = string.gsub(script, "@@1@@", appname)
  if jit.arch == "x86" then
    script = string.gsub(script, "@@2@@", ";")
  else
    script = string.gsub(script, "@@2@@", "")
  end
  return script
end
