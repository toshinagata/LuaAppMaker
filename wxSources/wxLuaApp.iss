#define MyAppName "wxLuaApp"
#define Version "(v1.0b3)"
[Setup]
AppName = {#MyAppName}
AppVerName = {#MyAppName} [#Version}
DefaultDirName = {pf}\{#MyAppName}
DefaultGroupName = {#MyAppName}
UninstallDisplayIcon = {app}\{#MyAppName}.exe
OutputBaseFileName = Setup_{#MyAppName}

[Files]
Source: "build\release\{#MyAppName}\{#MyAppName}.exe"; DestDir: {app}

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppName}.exe"
