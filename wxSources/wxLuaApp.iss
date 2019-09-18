#define MyAppName "wxLuaApp"
#define Version ""

[Setup]
AppName = {#MyAppName}
AppVerName = {#MyAppName} [#Version}
DefaultDirName = {pf}\{#MyAppName}
DefaultGroupName = {#MyAppName}
UninstallDisplayIcon = {app}\{#MyAppName}.exe
OutputBaseFileName = Setup_{#MyAppName}
ArchitecturesInstallIn64BitMode = x64
SourceDir = ..\build-win
OutputDir = ..\_latest_binaries

[Files]
; Install 64-bit version if running in 64-bit mode, 32-bit version otherwise.

; x86_64 files
Source: "build\release\{#MyAppName}\{#MyAppName}.exe"; DestDir: {app}; Check: Is64BitInstallMode
Source: "build\release\{#MyAppName}\lua51.dll"; DestDir: {app}; Check: Is64BitInstallMode
Source: "build\release\{#MyAppName}\lib\*"; DestDir: {app}\lib; Check: Is64BitInstallMode

; i386 files (the first one should be marked 'solidbreak')
Source: "..\build-win32\build\release\{#MyAppName}\{#MyAppName}.exe"; DestDir: {app}; Check: Is64BitInstallMode; Flags: solidbreak
Source: "..\build-win32\build\release\{#MyAppName}\lua51.dll"; DestDir: {app}; Check: not Is64BitInstallMode
Source: "..\build-win32\build\release\{#MyAppName}\lib\*"; DestDir: {app}\lib; Check: not Is64BitInstallMode

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppName}.exe"
Name: "{group}\Uninstall {#MyAppName}"; Filename: {uninstallexe}
