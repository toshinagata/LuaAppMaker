/////////////////////////////////////////////////////////////////////////////
// Purpose:     Standalone wxLua application
// Author:      John Labenski, Francis Irving, J. Winwood, Toshi Nagata
// Created:     16/01/2002
// Modified:    12/12/2019
// Copyright:   (c) 2012 John Labenski
// Copyright:   (c) 2002 Creature Labs. All rights reserved.
// Copyright:   (c) 2001-2002 Lomtick Software. All rights reserved.
// Copyright:   (c) 2019 Toshi Nagata
// Licence:     wxWidgets licence
/////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#ifdef __WXGTK__
#include <locale.h>
#endif

#ifdef __WXMAC__
#include <CoreFoundation/CoreFoundation.h>
#include <sys/param.h>  /*  For MAXPATHLEN  */
#endif

#include <wx/cmdline.h>
#include <wx/fs_mem.h>
#include <wx/image.h>
#include <wx/filefn.h>

#include "wxlua/wxlua.h"
#include "wxlua/debugger/wxldserv.h"
#include "wxbind/include/wxcore_bind.h"

#include "LuaAppMaker.h"
#include "ConsoleFrame.h"

#include "lua_addition.h"

#include "art/wxlualogo.xpm"

extern "C"
{
#include "lualib.h"
}

//  For stat or _stat functions
#if defined(__WXMSW__)
#include <sys/types.h>
#include <sys/stat.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

//  For handling resource files
#if defined(__WXMSW__)
#include "win_resources.h"
#endif

#ifndef wxICON_NONE
#define wxICON_NONE 0 // for 2.8 compat
#endif

// Declare the binding initialization functions
// Note : We could also do this "extern bool wxLuaBinding_XXX_init();" and
//        later call "wxLuaBinding_XXX_init();" to initialize it.
//        However we use the macros that include #if conditions to have a
//        better chance of determining what libs are available.
// Note : Make sure you link to the binding libraries.

#include "wxbind/include/wxbinddefs.h"
WXLUA_DECLARE_BIND_ALL

static ConsoleFrame *sConsoleFrame = NULL;

// ---------------------------------------------------------------------------
// wxLuaStandaloneApp
// ---------------------------------------------------------------------------
IMPLEMENT_APP(wxLuaStandaloneApp)

wxDEFINE_EVENT(LUAAPP_EVENT, wxCommandEvent);
const int LuaAppEvent_openFiles = 1;
const int LuaAppEvent_executeLuaScript = 2;

bool gInitCompleted = false;
int gNumberOfOpenedFiles = 0;

/*
 *  Utility function
 *  Filename handling
 */
#if defined(__WXMSW__)
#include <windows.h>
void
translate_char(char *p, int from, int to)
{
    while (*p != 0) {
        if ((unsigned char)*p == from)
            *p = to;
        // p = CharNext(p);
        p++;
    }
}
void
fix_dosish_path(char *p)
{
    translate_char(p, '/', '\\');
}
#else
void
translate_char(char *p, int from, int to)
{
}
void
fix_dosish_path(char *p)
{
}
#endif

//  Cache the results of FindResourcePath()
static wxString *resourcePath = NULL;
static wxString *applicationName = NULL;

//  Find the path of the directory where the relevant resources are to be found.
//  Mac: the "Resources" directory in the application bundle.
//  Windows: the directory in which the application executable is located.
//  UNIX: ?
static wxString
FindResourcePath()
{
#if defined(__WXMAC__)
    
    if (resourcePath != NULL)
        return *resourcePath;
    
    resourcePath = new wxString;
    applicationName = new wxString;

    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFStringRef exeName = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(mainBundle, kCFBundleExecutableKey);
    if (exeName != NULL) {
        wxString appName(CFStringGetCStringPtr(exeName, kCFStringEncodingUTF8));
        *applicationName = appName;
        CFRelease(exeName);
    }
    CFURLRef ref = CFBundleCopyResourcesDirectoryURL(mainBundle);
    if (ref != NULL) {
        UInt8 buffer[MAXPATHLEN];
        if (CFURLGetFileSystemRepresentation(ref, true, buffer, sizeof buffer)) {
            wxString dirname((const char *)buffer);
            CFRelease(ref);
            *resourcePath = dirname;
            return dirname;
        }
        CFRelease(ref);
    }
    return wxEmptyString;

#elif defined(__WXMSW__)

    wxString str;
    wxString argv0 = wxTheApp->argv[0];

    if (resourcePath != NULL)
        return *resourcePath;
    
    resourcePath = new wxString;
    applicationName = new wxString;
    
    //  Fix dosish path (when invoked from MSYS console, the path may be unix-like)
    //  Note: absolute paths like /c/Molby/... (== c:\Molby\...) is not supported
    {
        char *p = strdup(argv0.utf8_str());
        fix_dosish_path(p);
        wxString argv0_fixed(p, wxConvFile);
        argv0 = argv0_fixed;
    }
    //  Is it an absolute path?
    if (wxIsAbsolutePath(argv0)) {
        str = argv0;
        goto found;
    } else {
        //  Is it a relative path?
        wxString currentDir = wxGetCwd();
        if (currentDir.Last() != wxFILE_SEP_PATH)
            currentDir += wxFILE_SEP_PATH;
        str = currentDir + argv0;
        if (wxFileExists(str)) {
            goto found;
        }
    }
    //  Search PATH
    {
        wxPathList pathList;
        pathList.AddEnvList(wxT("PATH"));
        str = pathList.FindAbsoluteValidPath(argv0);
        if (!str.IsEmpty()) {
            goto found;
        }
    }
    return wxEmptyString;
found:
    wxFileName fname(str);
    *resourcePath = fname.GetPath();
    *applicationName = fname.GetName();
    return *resourcePath;
#else
#error "FindResourcePath is not defined for UNIXes."
#endif
}

static wxString
FindApplicationName()
{
    if (applicationName != NULL)
        return *applicationName;
    FindResourcePath();
    if (applicationName != NULL)
        return *applicationName;
    else return wxEmptyString;
}

static wxString
MakeLuaString(wxString str)
{
    wxString::const_iterator i;
    wxString res = wxT("\"");
    for (i = str.begin(); i != str.end(); ++i) {
        wxUniChar ch = *i;
        char buf[8];
        if (ch >= 0 && ch < 32) {
            sprintf(buf, "\\%03d", (char)ch);
            res.Append(buf);
        } else if (ch == '\\' || ch == '\"') {
            buf[0] = '\\';
            buf[1] = ch;
            buf[2] = 0;
            res.Append(buf);
        } else {
            res.Append(ch);
        }
    }
    res.Append(wxT("\""));
    return res;
}

inline static wxString
DyLibExtension()
{
#if defined(__WXMSW__)
    return wxT("dll");
#elif defined(__WXMAC__)
    return wxT("dylib");
#else
    return wxT("so");
#endif
}

bool wxLuaStandaloneApp::OnInit()
{
    m_programName       = argv[0];  // The name of this program
    m_print_stdout      = true;
    m_dos_stdout        = false;
    m_print_msgdlg      = false;
    m_want_console      = false;
    m_mem_bitmap_added  = false;
    m_numberOfProcessedFiles = 0;
    m_wxmainExecuted    = false;
    
#if defined(__WXMSW__)
    m_checker = NULL;
    m_ipcServiceName = NULL;
    m_server = NULL;
    m_client = NULL;
#endif

    bool run_ok    = false; // has the Lua program executed ok
    bool dont_quit = false; // user specified -q switch to not quit
    bool debug = false;
    
#if defined(__WXMSW__) && wxCHECK_VERSION(2, 3, 3)
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(1, 1);
    WSAStartup(wVersionRequested, &wsaData);
#endif // defined(__WXMSW__) && wxCHECK_VERSION(2, 3, 3)

#if __WXMSW__
    if (!CheckInstance())
        return false;
#endif
    
    wxInitAllImageHandlers();
    wxFileSystem::AddHandler(new wxMemoryFSHandler);
    wxMemoryFSHandler::AddFile(wxT("wxLua"), wxBitmap(wxlualogo_xpm), wxBITMAP_TYPE_XPM);
    m_mem_bitmap_added = true;
 
    Connect(LuaAppEvent_openFiles, LUAAPP_EVENT, wxCommandEventHandler(wxLuaStandaloneApp::OnOpenFilesByEvent));
    Connect(LuaAppEvent_executeLuaScript, LUAAPP_EVENT, wxCommandEventHandler(wxLuaStandaloneApp::OnExecuteLuaScript));
    Connect(wxID_EXIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(wxLuaStandaloneApp::OnQuitCommand));
    Connect(MyID_CREATE_APP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(wxLuaStandaloneApp::OnCreateApplication));
    Connect(wxEVT_LUA_PRINT, wxLuaEventHandler(wxLuaStandaloneApp::OnLua));
    Connect(wxEVT_LUA_ERROR, wxLuaEventHandler(wxLuaStandaloneApp::OnLua));

#ifdef __WXGTK__
    // this call is very important since otherwise scripts using the decimal
    // point '.' could not work with those locales which use a different symbol
    // (e.g. the comma) for the decimal point...
    // It doesn't work to put os.setlocale('c', 'numeric') in the Lua file that
    // you want to use decimal points in. That's because the file has been lexed
    // and compiler before the locale has changed, so the lexer - the part that
    // recognises numbers - will use the old locale.
    setlocale(LC_NUMERIC, "C");
#endif
    
    // Initialize the wxLua bindings we want to use.
    // See notes for WXLUA_DECLARE_BIND_ALL above.
    WXLUA_IMPLEMENT_BIND_ALL
    
    // When this function returns wxApp:MainLoop() will be called by wxWidgets
    // and so we want the Lua code wx.wxGetApp:MainLoop() to not
    // prematurely start it.
    wxLuaState::sm_wxAppMainLoop_will_run = true;
    
    m_wxlState = wxLuaState(this, wxID_ANY);
    if (!m_wxlState.Ok())
        return false;
    
    //  Define extended string methods
    lua_State *L = m_wxlState.GetLuaState();
    lua_register_string_ext(L);
    
    //  Initialize "LuaApp" as a shortcut to wx.wxGetApp()
    //  LuaApp.config is defined as an empty table (which may be overwritten later)
    m_wxlState.RunString(wxT("LuaApp = wx.wxGetApp(); LuaApp.config = {}"));
    
    //  Replace package.path and package.cpath (to prevent looking outside this app)
    wxString rpath = FindResourcePath();
    wxString lpath = wxT("./?.lua;") +
        rpath + wxT("/scripts/?.lua;") +
        rpath + wxT("/lib/?.lua");
    lpath.Replace(wxT("/"), wxFILE_SEP_PATH, true);
    wxString ext = DyLibExtension();
    //  On Win32 system, "./lib32" directory is searched before "./", and
    //  "$(rpath)/lib32/" is searched before "$(rpath)/lib/".
    //  This allows dlls of the same name can be used for both 64bit and 32bit systems.
    wxString cpath =
#if defined(__WXMSW__) && defined(__i386__)
        wxT("./lib32/?.lua;");
#else
        wxT("");
#endif
    cpath =
        cpath + wxT("./?.lua;") +
        rpath + wxT("/scripts/?.") + ext + wxT(";") +
#if defined(__WXMSW__) && defined(__i386__)
        rpath + wxT("/lib32/?.") + ext +
#endif
        rpath + wxT("/lib/?.") + ext;
    cpath.Replace(wxT("/"), wxFILE_SEP_PATH, true);
    
    m_wxlState.RunString(wxT("package.path = ") + MakeLuaString(lpath));
    m_wxlState.RunString(wxT("package.cpath = ") + MakeLuaString(cpath));

    //  Configuration file path
    wxString confDirPath = wxStandardPaths::Get().GetUserConfigDir() + wxFILE_SEP_PATH + wxT("LuaAppMaker");
    if (!wxDirExists(confDirPath)) {
        //  Try to create the configuration directory
        //  (Even if it fails, we will just ignore)
        wxMkdir(confDirPath);
    }
    m_wxlState.RunString(wxT("LuaApp.settingsDirPath = ") + MakeLuaString(confDirPath));
    
    //  Settings file: may be replaced later
    wxString confPath = confDirPath + wxFILE_SEP_PATH + FindApplicationName() + wxT("_conf.lua");
    m_wxlState.RunString(wxT("LuaApp.settingsPath = ") + MakeLuaString(confPath));

    //  Resource path and application name
    //  (May be replaced later)
    m_wxlState.RunString(wxT("LuaApp.resourcePath = ") + MakeLuaString(FindResourcePath()));
    m_wxlState.RunString(wxT("LuaApp.applicationName = ") + MakeLuaString(FindApplicationName()));
    
    //  Build date and version string
    m_wxlState.RunString(wxT("LuaApp.buildDate = ") + MakeLuaString(gLastBuildString));
    m_wxlState.RunString(wxT("LuaApp.version = ") + MakeLuaString(gVersionString));

    //  Import startup definition
    wxString conf = rpath + wxFILE_SEP_PATH + wxT("lib") + wxFILE_SEP_PATH + wxT("startup.lua");
    if (wxFileExists(conf)) {
        int rc = m_wxlState.RunFile(conf);
        run_ok = (rc == 0);
    }

    //  Start console
    m_want_console = true;
    sConsoleFrame = ConsoleFrame::CreateConsoleFrame(NULL);
    SetTopWindow(sConsoleFrame);
    sConsoleFrame->Show(true);

    //  Set LuaApp.luaConsole as the console frame
    wxFrame *consoleFrame = wxDynamicCast(sConsoleFrame, wxFrame);
    if (consoleFrame != NULL) {
        lua_getglobal(L, "LuaApp");
        wxluaT_pushuserdatatype(L, consoleFrame, wxluatype_wxFrame);
        lua_setfield(L, -2, "luaConsole");
        lua_pop(L, 1);
    }

    // check to see if there is a toplevel window open for the user to close, else exit
    if (run_ok && !dont_quit)
    {
        if (GetTopWindow() == NULL)
            run_ok = false;
    }
    
    //  Open given files
    //  The file open mechanism is also used by IPC, so the arguments
    //  must be combined as a single string
    wxString files;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-debug") == 0) {
            debug = true;
        }
        files.append(argv[i]);
        files.append(wxT("\n"));
    }
    
    if (debug) {
        //  Import miniDebug client
        m_wxlState.RunString(wxT("LuaApp.debug = require(\"miniDebug\")"));
    }
    
    //  In the Lua world, create a global table "arg"
    //  and set arg[-1] the path of this application
    lua_newtable(L);
    if (argc > 0) {
        wxString argv0(argv[0]);
        lua_pushstring(L, (const char *)argv0);
        lua_rawseti(L, -2, -1);
    }
    lua_setglobal(L, "arg");

    //  If LUA_PATH or LUA_CPATH is defined, then add it
    m_wxlState.RunString(wxT("local p = os.getenv('LUA_PATH'); package.path = ((p and p..';') or '')..package.path"));
    m_wxlState.RunString(wxT("local p = os.getenv('LUA_CPATH'); package.cpath = ((p and p..';') or '')..package.cpath"));
    
#if defined(__WXMAC__)
    //  Record the arguments (these should be ignored in MacOpenFiles)
    m_filesGivenByArgv.Clear();
    for (int i = 0; i < argc; i++) {
        wxString argvi(argv[i]);
        m_filesGivenByArgv.Add(argvi);
    }
#endif

    RequestOpenFilesByEvent(files);
    
    gInitCompleted = true;

    return true;
}

#if __WXMSW__
bool wxLuaStandaloneApp::CheckInstance()
{
    //  Check if the same application is already running
    char *buf, *p;
    wxString name = FindApplicationName() + wxT("-") + wxGetUserId();
    m_ipcServiceName = new wxString(name);
    m_ipcServiceName->Prepend(wxT("IPC-"));
    m_checker = new wxSingleInstanceChecker(name);
    if (m_checker->IsAnotherRunning()) {
        //  Make a connection with the other instance and ask for opening the file(s)
        if (argc > 1) {
            wxString files;
            wxConnectionBase *connection;
            int i;
            for (i = 1; i < argc; i++) {
                files.append(argv[i]);
                files.append(wxT("\n"));
            }
            m_client = new MyClient;
            connection = m_client->MakeConnection(wxT("localhost"), *m_ipcServiceName, MYAPP_IPC_TOPIC);
            if (connection == NULL) {
                wxLogError(wxT("My application is already running; please shut it down and retry"));
                delete m_client;
                return false;
            }
            connection->Execute(files);
            delete m_client;
        }
        return false;
    } else {
        m_server = new MyServer;
        if (m_server->Create(*m_ipcServiceName) == false) {
            delete m_server;
            m_server = NULL;
        }
    }
    return true;
}
#endif

int wxLuaStandaloneApp::OnExit()
{
    m_wxlState.CloseLuaState(true);
    m_wxlState.Destroy();
    
    m_want_console = false; // no more messages
    
    if (wxLuaConsole::HasConsole())
    {
        wxLuaConsole::GetConsole()->Destroy();
    }
    
    if (m_mem_bitmap_added)
        wxMemoryFSHandler::RemoveFile(wxT("wxLua"));
    
    wxSafeYield();  // make sure windows get destroyed
    
    int ret = wxApp::OnExit();
    
#if defined(__WXMSW__)
    delete m_checker;
    delete m_server;
#endif

#if defined(__WXMSW__) && wxCHECK_VERSION(2, 3, 3)
    WSACleanup ();
#endif // defined(__WXMSW__) && wxCHECK_VERSION(2, 3, 3)
    
    return ret;
}

void wxLuaStandaloneApp::DisplayMessage(const wxString &msg, bool is_error)
{
    if (sConsoleFrame == NULL)
        return;

    if (!is_error) {
        sConsoleFrame->AppendConsoleMessage((msg + wxT("\n")).utf8_str());
    } else {
        sConsoleFrame->SetConsoleColor(1);
        if (!m_wxlState.IsOk()) {
            sConsoleFrame->SetConsoleColor(0);
            return;
        }
        sConsoleFrame->AppendConsoleMessage((msg + wxT("\n")).utf8_str());
        lua_Debug luaDebug = INIT_LUA_DEBUG;
        lua_State* L = m_wxlState.GetLuaState();
        int nIndex = 0;
        wxString buffer;
        while (lua_getstack(L, nIndex, &luaDebug) != 0) {
            if (lua_getinfo(L, "Sln", &luaDebug)) {
                wxString what    (luaDebug.what     ? lua2wx(luaDebug.what)     : wxString(wxT("?")));
                wxString nameWhat(luaDebug.namewhat ? lua2wx(luaDebug.namewhat) : wxString(wxT("?")));
                wxString name    (luaDebug.name     ? lua2wx(luaDebug.name)     : wxString(wxT("?")));
                buffer += wxString::Format(wxT("[%d] %s '%s' '%s' (line %d)\n    Line %d src='%s'\n"),
                                           nIndex, what.utf8_str(), nameWhat.utf8_str(), name.utf8_str(), luaDebug.linedefined,
                                           luaDebug.currentline, lua2wx(luaDebug.short_src).utf8_str());
            }
            nIndex++;
        }
        if (!buffer.empty()) {
            wxString msg
            = wxT("\n-----------------------------------------------------------"
                  "\n- Backtrace"
                  "\n-----------------------------------------------------------\n")
            + buffer +
              wxT("\n-----------------------------------------------------------\n\n");
            sConsoleFrame->AppendConsoleMessage(msg.utf8_str());
        }
        sConsoleFrame->SetConsoleColor(0);
        if (!sConsoleFrame->IsShown()) {
            //  Cancel is 'Show Lua Console' and quit this dialog
            wxMessageDialog *dlg = new wxMessageDialog(NULL, msg, wxT("Internal Error"), wxOK | wxCANCEL);
            dlg->SetOKCancelLabels(wxID_OK, wxT("Show Lua Console"));
            if (dlg->ShowModal() == wxID_CANCEL) {
                sConsoleFrame->Show();
                sConsoleFrame->Raise();
            }
            dlg->Destroy();
        }
    }
}

void wxLuaStandaloneApp::OnLua( wxLuaEvent &event )
{
    DisplayMessage(event.GetString(), event.GetEventType() == wxEVT_LUA_ERROR);
}

void wxLuaStandaloneApp::OnQuitCommand(wxCommandEvent &event)
{
    wxTopLevelWindow **windows;
    wxWindowList::iterator iter;
    int i, n = wxTopLevelWindows.GetCount();
    windows = (wxTopLevelWindow **)malloc(n * sizeof(wxTopLevelWindow*));
    if (windows == NULL)
        return;
    //  Pass 1: get the list of all windows
    for (iter = wxTopLevelWindows.begin(), i = 0; iter != wxTopLevelWindows.end(); ++iter, i++) {
        windows[i] = (wxTopLevelWindow *)(*iter);
    }
    //  Pass 2: close all windows except for LuaConsole
    for (i = 0; i < n; i++) {
        if (sConsoleFrame != NULL && windows[i] == wxDynamicCast(sConsoleFrame, wxWindow))
            continue;  //  Skip this
        if (windows[i]->IsKindOf(wxCLASSINFO(wxDialog))) {
            //  Dialogs will be destroyed without wxCloseEvent
            if (!windows[i]->Destroy())
                return;  /*  Cannot close this dialog  */
        } else {
            if (!windows[i]->Close())
                return;  /*  Cannot close this window  */
        }
    }
    //  Pass 3: close LuaConsole
    if (sConsoleFrame != NULL) {
        sConsoleFrame->Destroy();
        sConsoleFrame = NULL;
    }
    free(windows);
}

void wxLuaStandaloneApp::OnExecuteLuaScript(wxCommandEvent &event)
{
    wxString str = event.GetString();
    m_wxlState.RunString(str, wxT(""), 0);
}

#if 0
#pragma mark ====== Open Files by Drag & Drop ======
#endif

void
wxLuaStandaloneApp::RequestOpenFilesByEvent(wxString& files)
{
    //  Open files by IPC request
    int start, end;
    start = 0;
    while (1) {
        end = files.find(wxT("\n"), start);
        wxString file = files.Mid(start, (end == wxString::npos ? wxString::npos : end - start));
        if (file.Len() == 0)
            break;
        m_pendingFilesToOpen.Add(file);
        if (end == wxString::npos)
            break;
        start = end + 1;
    }
    wxCommandEvent myEvent(LUAAPP_EVENT, LuaAppEvent_openFiles);
    wxPostEvent(this, myEvent);
}

void
wxLuaStandaloneApp::OnOpenFilesByEvent(wxCommandEvent& event)
{
    if (!gInitCompleted) {
        //  Repost this event and try again later
        wxCommandEvent myEvent(LUAAPP_EVENT, LuaAppEvent_openFiles);
        wxPostEvent(this, myEvent);
        return;
    }
    OpenPendingFiles();
}

bool
wxLuaStandaloneApp::CheckLuaLogicalExpression(wxString str)
{
    bool retval;
    m_wxlState.RunString(wxT("return ") + str, wxT(""), 1);
    retval = m_wxlState.lua_ToBoolean(-1);
    m_wxlState.lua_Pop(1);
    return retval;
}

bool
wxLuaStandaloneApp::OpenPendingFiles()
{
    lua_State *L = m_wxlState.GetLuaState();
    int hideConsole = 0;

    if (!m_wxmainExecuted) {

        bool isEmbedded = false;
        wxString appName;

        //  First invocation: we need to look for the wxmain.lua to execute
        //  Embedded script
        wxFileName dname(FindResourcePath(), wxT("scripts"));
        dname.MakeAbsolute();
        wxString dpath = dname.GetFullPath();
        wxString spath = dpath + wxFILE_SEP_PATH + wxT("wxmain.lua");
        if (wxFileName::DirExists(dpath) && wxFileName::FileExists(spath)) {
            isEmbedded = true;
        } else {
            dpath = wxT("");
        }

        //  Look for the script file
        //  (Only for the first given argument)
        wxString dpath1;
        if (!isEmbedded && m_pendingFilesToOpen.GetCount() > 0) {
            wxFileName dname1(m_pendingFilesToOpen[0]);
            dname1.MakeAbsolute();
            dpath1 = dname1.GetFullPath();
            wxString spath1 = dpath1 + wxFILE_SEP_PATH + wxT("wxmain.lua");
            if (wxFileName::DirExists(dpath1) && wxFileName::FileExists(spath1)) {
                //  Directory containing 'wxmain.lua'
                dpath = dpath1;
                spath = spath1;
                //  Regard the directory name as the application name
                appName = dname1.GetName();
                m_pendingFilesToOpen.RemoveAt(0, 1);
            } else if (dpath1.EndsWith(wxT(".lua")) && wxFileName::FileExists(dpath1)) {
                //  A single file '*.lua'
                spath = dpath1;
                dpath = dname1.GetPath();
                m_pendingFilesToOpen.RemoveAt(0, 1);
                //  Regard the base name as the application name
                appName = dname1.GetName();
            } else if (wxFileName::DirExists(dpath1)) {
                //  Directory containing a single file '*.lua'
                wxDir dir(dpath1);
                wxString fname;
                bool found = dir.GetFirst(&fname, wxT("*.lua"));
                if (found) {
                    spath1 = dpath1 + wxFILE_SEP_PATH + fname;
                    found = !dir.GetNext(&fname);  //  Needs to contain only one *.lua file
                }
                if (found) {
                    dpath = dpath1;
                    spath = spath1;
                    m_pendingFilesToOpen.RemoveAt(0, 1);
                    //  Regard the directory name as the application name
                    appName = dname1.GetName();
                }
            }
        }
        if (wxStrcmp(dpath, wxT("")) == 0) {
            //  Wait for a script file to be given
            static bool waitingMessage = false;
            if (!waitingMessage) {
                DisplayMessage(wxT("Waiting for the directory containing wxmain.lua to be given."), false);
                waitingMessage = true;
            }
            m_pendingFilesToOpen.Clear();
            return false;
        }

        //  Build 'arg' table in the Lua world
        lua_getglobal(L, "arg");
        lua_pushstring(L, (const char *)dpath1);
        lua_rawseti(L, -2, 0);
        for (int i = 0; i < m_pendingFilesToOpen.GetCount(); i++) {
            wxString f = m_pendingFilesToOpen[i];
            lua_pushstring(L, (const char *)f);
            lua_rawseti(L, -2, i + 1);
        }
        lua_pop(L, 1);

        //  Replace application name, resource path and configuration file
        if (wxStrcmp(appName, wxT("")) != 0) {
            wxString confName = wxFILE_SEP_PATH + appName + wxT("_conf.lua");
            m_wxlState.RunString(wxT("LuaApp.settingsPath = LuaApp.settingsDirPath .. ") + MakeLuaString(confName));
            m_wxlState.RunString(wxT("LuaApp.applicationName = ") + MakeLuaString(appName));
            m_wxlState.RunString(wxT("LuaApp.resourcePath = ") + MakeLuaString(dpath));
        }

        //  Try to load wxmain.lua
        //  Set the working directory
        wxSetWorkingDirectory(dpath);
        //  Set the package search directory
        wxString lpath = dpath + wxFILE_SEP_PATH + wxT("?.lua;");
        wxString cpath =
#if defined(__WXMSW__) && defined(__i386__)
        dpath + wxFILE_SEP_PATH + wxT("lib32\\?.") + DyLibExtension() + wxT(";") +
#endif
        dpath + wxFILE_SEP_PATH + wxT("?.") + DyLibExtension() + wxT(";");
        m_wxlState.RunString(wxT("package.path = ") + MakeLuaString(lpath) + wxT(".. package.path"));
        m_wxlState.RunString(wxT("package.cpath = ") + MakeLuaString(cpath) + wxT(".. package.cpath"));
        
        //  Load "wxmain.lua"
        DisplayMessage(wxT("Running wxmain.lua from " + dpath), false);
        int rc = m_wxlState.RunFile(spath);
        if (rc == 0) {
            if (!CheckLuaLogicalExpression(wxT("LuaApp.config.showConsole"))) {
                hideConsole = 1;
            }
            //  Start debugger client (if not started yet)
            m_wxlState.RunString(wxT("if LuaApp.debug then LuaApp.debug.start() end"));
            m_wxmainExecuted = true;
        } else {
            DisplayMessage(wxlua_LUA_ERR_msg(rc), false);
        }
        //  Define LuaApp.Reload
        //  (This function reloads the given lua file, even if RunFile(spath) failed)
        m_wxlState.RunString(wxT("function LuaApp.Reload() dofile(") + MakeLuaString(spath) + ") end");

        if (!m_wxmainExecuted) {
            DisplayMessage(wxT("Waiting for a directory containing wxmain.lua"), false);
            m_pendingFilesToOpen.Clear();
            return false;
        }
    }

    if (CheckLuaLogicalExpression(wxT("LuaApp.OpenMultipleDocuments"))) {
        //  Is LuaApp.OpenMultipleDocuments defined?
        //  If yes, then call it with all files as a single argument
        wxString args = wxT("({");
        int n = m_pendingFilesToOpen.GetCount();
        for (int i = 0; i < n; i++) {
            args = args + MakeLuaString(m_pendingFilesToOpen[i]);
            if (i < n - 1)
                args = args + wxT(",");
        }
        args = args + wxT("})");
        m_wxlState.RunString(wxT("LuaApp.OpenMultipleDocuments") + args);
    } else if (CheckLuaLogicalExpression(wxT("LuaApp.OpenDocument"))) {
        //  Is LuaApp.OpenDocument defined?
        //  If yes, then call it for each files
        //  (except for those beginning with a '-', which is probably an option)
        for (int i = 0; i < m_pendingFilesToOpen.GetCount(); i++) {
            wxString f = m_pendingFilesToOpen[i];
            if (wxStrncmp(f, wxT("-"), 1) != 0) {
                //  Open file: call LuaApp.OpenDocument if defined
                m_wxlState.RunString(wxT("LuaApp.OpenDocument(") + MakeLuaString(f) + wxT(")"));
                m_numberOfProcessedFiles++;
            }
        }
        if (m_numberOfProcessedFiles == 0) {
            //  We will call LuaApp.NewDocument() if defined
            if (CheckLuaLogicalExpression(wxT("LuaApp.NewDocument"))) {
                m_wxlState.RunString(wxT("LuaApp.NewDocument()"));
                m_numberOfProcessedFiles++;
            }
        }
    }
    m_pendingFilesToOpen.Clear();

    if (hideConsole) {
        //  Hide the console if any other window is open
        int count = wxTopLevelWindows.GetCount();
        if (count > 1) {
            wxCommandEvent *myEvent = new wxCommandEvent(LUAAPP_EVENT, LuaAppEvent_executeLuaScript);
            myEvent->SetString(wxT("LuaApp.luaConsole:Hide()"));
            this->QueueEvent(myEvent);
        }
    }
    sConsoleFrame->ShowPrompt();
    return true;
}

void
wxLuaStandaloneApp::MacOpenFiles(const wxArrayString &fileNames)
{
    int i, j, size;
    wxString files;
    size = fileNames.size();
    for (i = 0; i < size; i++) {
#if defined(__WXMAC__)
        for (j = m_filesGivenByArgv.GetCount() - 1; j >= 0; j--) {
            if (wxStrcmp(fileNames[i], m_filesGivenByArgv[j]) == 0)
                break;
        }
        if (j >= 0) {
            //  Ignore this file, and remove it from m_filesGivenByArgv[j]
            m_filesGivenByArgv.RemoveAt(j);
            continue;
        }
#endif
        m_pendingFilesToOpen.Add(fileNames[i]);
    }
    OpenPendingFiles();
}

//  We need this to be overridden, because we want OpenPendingFiles() to be
//  called with an empty file list on startup.
void
wxLuaStandaloneApp::MacNewFile()
{
    OpenPendingFiles();
}

#if 0
#pragma mark ====== Create Application ======
#endif

static void
SetFileExecutable(wxString &fullpath)
{
#if defined(__WXMSW__)
    //  Do nothing: Windows has no executable bits
#else
    const char *cpath = fullpath.utf8_str();
    struct stat fileStat;
    mode_t newmode;
    if(stat(cpath, &fileStat) < 0)
        return;
    newmode = fileStat.st_mode | S_IXUSR | S_IXGRP | S_IXOTH;
    chmod(cpath, newmode);
#endif
}

/*  Copy the destination recursively  */
/*  If the filename begins with '_' or '.', then it is skipped  */
static wxString
CopyRecursive(wxString &src, wxString &dst, bool allowOverwrite)
{
    if (wxFileName::DirExists(src)) {
        if (wxFileName::FileExists(dst)) {
            return wxString::Format(wxT("Cannot copy directory %s over file %s"), (const char *)src, (const char *)dst);
        } else if (!wxFileName::DirExists(dst)) {
            //  Create a directory
            wxFileName dname = wxFileName::DirName(dst);
            dname.Mkdir();
        }
        wxDir dir(src);
        if (!dir.IsOpened())
            return wxString::Format(wxT("Cannot open directory %s"), (const char *)src);
        wxString name;
        if (dir.GetFirst(&name)) {
            do {
                if (!name.StartsWith(wxT("_")) && !name.StartsWith(wxT("."))) {
                    wxString src1 = src + wxFILE_SEP_PATH + name;
                    wxString dst1 = dst + wxFILE_SEP_PATH + name;
                    CopyRecursive(src1, dst1, allowOverwrite);
                }
            } while (dir.GetNext(&name));
        }
    } else if (wxFileName::FileExists(src)) {
        //  Copy file from src to dst
        if (wxFileName::DirExists(dst)) {
            return wxString::Format(wxT("Cannot overwrite file on directory %s"), (const char *)dst);
        } else if (wxFileName::Exists(dst) && !allowOverwrite) {
            return wxString::Format(wxT("Cannot overwrite file on %s"), (const char *)dst);
        }
        wxFile srcFile(src);
        wxFile dstFile(dst, wxFile::write);
        ssize_t ssize;
        char *buf;
        const ssize_t bufsize = 128*1024;
        buf = (char *)malloc(bufsize);
        if (buf == NULL)
            return wxString::Format(wxT("Cannot allocate buffer during copy of %s"), (const char *)src);
        while ((ssize = srcFile.Read(buf, bufsize)) > 0) {
            dstFile.Write(buf, ssize);
        }
        srcFile.Close();
        dstFile.Close();
        //  Change times and executable bit
        wxFileName srcFName(src);
        wxFileName dstFName(dst);
        wxDateTime dtAccess, dtMod, dtCreate;
        if (srcFName.GetTimes(&dtAccess, &dtMod, &dtCreate))
            dstFName.SetTimes(&dtAccess, &dtMod, &dtCreate);
        if (srcFName.IsFileExecutable())
            SetFileExecutable(dst);
    } else {
        return wxString::Format(wxT("File %s does not exist"), (const char *)src);
    }
    return wxT("");
}

void
wxLuaStandaloneApp::OnCreateApplication(wxCommandEvent &event)
{
    lua_State *L = m_wxlState.GetLuaState();
    const char *script_folder = NULL;
    const char *app_name = NULL;
    const char *icon_path = NULL;
    //  LuaApp.CreateApp() returns the following info:
    //  1 script_folder (string)
    //  2 app_name (string)
    //  3 icon_path (string)
    m_wxlState.RunString(wxT("require \"create_app\"; return LuaApp.CreateApp()"), wxT(""), 3);
    if (!lua_isnil(L, -3))
        script_folder = lua_tolstring(L, -3, NULL);
    if (!lua_isnil(L, -2))
        app_name = lua_tolstring(L, -2, NULL);
    if (!lua_isnil(L, -1))
        icon_path = lua_tolstring(L, -1, NULL);
    lua_pop(L, 3);
    if (script_folder == NULL || script_folder[0] == 0)
        return;  /*  Do nothing  */
    if (icon_path == NULL || icon_path[0] == 0)
        return;  /*  Do nothing  */
    wxString scriptFolder(script_folder);
    wxString iconPath(icon_path);
    
    //  Replace the script folder name with app_name
    wxFileName scriptFolderFName = wxFileName::DirName(scriptFolder);
    scriptFolderFName.RemoveLastDir();
    wxString appName(app_name);
#if defined(__WXMAC__)
    wxString appNameExt = appName + wxT(".app");
#else
    wxString appNameExt = appName;
#endif
    
    int n = 1;
    while (1) {
        if (!scriptFolderFName.AppendDir(appNameExt)) {
            DisplayMessage(wxT("ERROR: Cannot build the application folder name"), false);
            return;
        }
        if (!scriptFolderFName.Exists())
            break;  //  OK
        scriptFolderFName.RemoveLastDir();
        n++;
#if defined(__WXMAC__)
        appNameExt = wxString::Format("%s %d.app", app_name, n);
#else
        appNameExt = wxString::Format("%s %d", app_name, n);
#endif
    }
    wxString appDstPath = scriptFolderFName.GetFullPath();
    
#if defined(__WXMAC__)
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef ref = CFBundleCopyBundleURL(mainBundle);
    UInt8 app_src_path[MAXPATHLEN];
    app_src_path[0] = 0;
    if (ref != NULL) {
        CFURLGetFileSystemRepresentation(ref, true, app_src_path, sizeof app_src_path);
        CFRelease(ref);
    }
    if (app_src_path[0] == 0) {
        DisplayMessage(wxT("ERROR: Cannot get application path"), false);
        return;
    }
    wxString appSrcPath(app_src_path);
    CopyRecursive(appSrcPath, appDstPath, true);
    //  Copy script folder
    wxString scriptDstPath = appDstPath + wxT("/Contents/Resources/scripts");
    CopyRecursive(scriptFolder, scriptDstPath, true);
    //  Copy Icon
    wxString iconDstPath = appDstPath + wxT("/Contents/Resources/") + appName + wxT(".icns");
    CopyRecursive(iconPath, iconDstPath, true);
    //  Remove LuaAppMaker icon
    wxString iconOrigPath = appDstPath + wxT("/Contents/Resources/wxlualogo.icns");
    ::wxRemoveFile(iconOrigPath);
    //  Rewrite Info.plist
    wxString plistPath = appDstPath + wxT("/Contents/Info.plist");
    wxFile plist(plistPath);
    wxString plistContent;
    plist.ReadAll(&plistContent);
    plist.Close();
    plistContent.Replace(wxT("LuaAppMaker"), appName, true);
    plistContent.Replace(wxT("wxlualogo.icns"), appName + wxT(".icns"), true);
    wxFile plistOut(plistPath, wxFile::write);
    plistOut.Write(plistContent);
    plistOut.Close();
    //  Rename executable
    wxString exeName = appDstPath + wxT("/Contents/MacOS/LuaAppMaker");
    wxString exeNewName = appDstPath + wxT("/Contents/MacOS/") + appName;
    ::wxRenameFile(exeName, exeNewName);

#elif defined(__WXMSW__)
    //  Find the folder in which this executable is located
    wxFileName appSrcFName = wxFileName::DirName(FindResourcePath());
    wxString appSrcPath = appSrcFName.GetFullPath();

    //  Copy executable
    CopyRecursive(appSrcPath, appDstPath, true);
    //  Copy script folder
    wxString scriptDstPath = appDstPath + wxT("scripts");
    CopyRecursive(scriptFolder, scriptDstPath, true);
    //  Replace application icon
    wxString exeName = appDstPath + wxFILE_SEP_PATH + wxT("LuaAppMaker.exe");
    ReplaceWinAppIcon(exeName, iconPath);
    //  Rename executable
    wxString exeNewName = appDstPath + wxFILE_SEP_PATH + appName + wxT(".exe");
    ::wxRenameFile(exeName, exeNewName);
    //  Create script for Inno Setup
    m_wxlState.RunString(wxT("return LuaApp.CreateISS(") + MakeLuaString(appName) + wxT(")"), wxT(""), 1);
    const char *iss_script;
    if (!lua_isnil(L, -1)) {
        iss_script = lua_tolstring(L, -1, NULL);
        wxFile issFile(appDstPath + wxFILE_SEP_PATH + appName + wxT(".iss"), wxFile::write);
        if (issFile.IsOpened()) {
            issFile.Write(iss_script, strlen(iss_script));
            issFile.Close();
        }
    }
    lua_pop(L, 1);

#else
    DisplayMessage(wxT("script_folder = ") + wxString(script_folder ? script_folder : "(nil)") + wxT("\n"), false);
    DisplayMessage(wxT("icon_path = ") + wxString(icon_path ? icon_path : "(nil)") + wxT("\n"), false);
#endif
    return;
}

#if 0
#pragma mark ====== IPC Support (for Windows) ======
#endif

#if defined(__WXMSW__)

wxString *gIPCServiceName = NULL;

bool
MyClientConnection::OnDisconnect()
{
    wxGetApp().m_client->Disconnect();
}

MyClient::MyClient()
{
    m_clientConnection = NULL;
}

MyClient::~MyClient()
{
    Disconnect();
}

void
MyClient::Disconnect()
{
    if (m_clientConnection != NULL) {
        m_clientConnection->Disconnect();
        m_clientConnection = NULL;
    }
}

wxConnectionBase *
MyClient::OnMakeConnection()
{
    if (m_clientConnection == NULL)
        m_clientConnection = new MyClientConnection;
    return m_clientConnection;
}

bool
MyServerConnection::OnDisconnect()
{
    wxGetApp().m_server->Disconnect();
}

bool
MyServerConnection::OnExecute(const wxString& topic, const void *data, size_t size, wxIPCFormat format)
{
    if (topic == MYAPP_IPC_TOPIC) {
        wxString files((wxChar *)data);
        wxGetApp().RequestOpenFilesByEvent(files);
        return true;
    } else return false;
}

MyServer::MyServer()
{
    m_serverConnection = NULL;
}

MyServer::~MyServer()
{
    Disconnect();
}

void
MyServer::Disconnect()
{
    if (m_serverConnection != NULL) {
        m_serverConnection->Disconnect();
        m_serverConnection = NULL;
    }
}

wxConnectionBase *
MyServer::OnAcceptConnection(const wxString &topic)
{
    if (topic == MYAPP_IPC_TOPIC) {
        if (m_serverConnection == NULL)
            m_serverConnection = new MyServerConnection();
        return m_serverConnection;
    }
    return NULL;
}

#endif  // defined(__WXMSW__)
