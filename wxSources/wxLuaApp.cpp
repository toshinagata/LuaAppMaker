/////////////////////////////////////////////////////////////////////////////
// Purpose:     Standalone wxLua application
// Author:      John Labenski, Francis Irving, J. Winwood
// Created:     16/01/2002
// Copyright:   (c) 2012 John Labenski
// Copyright:   (c) 2002 Creature Labs. All rights reserved.
// Copyright:   (c) 2001-2002 Lomtick Software. All rights reserved.
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

#include "wxLuaApp.h"
#include "ConsoleFrame.h"

#include "lua_addition.h"

#include "art/wxlualogo.xpm"

extern "C"
{
#include "lualib.h"
//#include "luagl.h"    //  for LuaGL
//#include "luaglu.h"   //  for LuaGLU
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

//  Find the path of the directory where the relevant resources are to be found.
//  Mac: the "Resources" directory in the application bundle.
//  Windows: the directory in which the application executable is located.
//  UNIX: ?
static wxString
FindResourcePath()
{
#if defined(__WXMAC__)
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef ref = CFBundleCopyResourcesDirectoryURL(mainBundle);
    if (ref != NULL) {
        UInt8 buffer[MAXPATHLEN];
        if (CFURLGetFileSystemRepresentation(ref, true, buffer, sizeof buffer)) {
            wxString dirname((const char *)buffer);
            CFRelease(ref);
            return dirname;
        }
        CFRelease(ref);
    }
    return wxEmptyString;
/*    CFBundleRef mainBundle = CFBundleGetMainBundle();
    if (mainBundle != NULL) {
        CFURLRef ref = CFBundleCopyBundleURL(mainBundle);
        if (ref != NULL) {
            char buffer[MAXPATHLEN];
            CFURLRef ref2 = CFURLCreateCopyDeletingLastPathComponent(NULL, ref);
            if (ref2 != NULL) {
                if (CFURLGetFileSystemRepresentation(ref2, true, (UInt8 *)buffer, sizeof buffer)) {
                    wxString dirname((const char *)buffer);
                    CFRelease(ref2);
                    CFRelease(ref);
                    CFRelease(mainBundle);
                    return dirname;
                }
                CFRelease(ref2);
            }
            CFRelease(ref);
        }
        CFRelease(mainBundle);
    }
    return wxEmptyString;
*/
#elif defined(__WXMSW__)
    wxString str;
    wxString argv0 = wxTheApp->argv[0];
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
        return wxPathOnly(argv0);
    } else {
        //  Is it a relative path?
        wxString currentDir = wxGetCwd();
        if (currentDir.Last() != wxFILE_SEP_PATH)
            currentDir += wxFILE_SEP_PATH;
        str = currentDir + argv0;
        if (wxFileExists(str))
            return wxPathOnly(str);
    }
    //  Search PATH
    wxPathList pathList;
    pathList.AddEnvList(wxT("PATH"));
    str = pathList.FindAbsoluteValidPath(argv0);
    if (!str.IsEmpty())
        return wxPathOnly(str);
    return wxEmptyString;
#else
#error "FindResourcePath is not defined for UNIXes."
#endif
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
//    m_wxlDebugTarget    = NULL;
    m_mem_bitmap_added  = false;
    m_numberOfProcessedFiles = -1;
    
#if defined(__WXMSW__)
    m_checker = NULL;
    m_ipcServiceName = NULL;
    m_server = NULL;
    m_client = NULL;
#endif

    bool run_ok    = false; // has the Lua program executed ok
    bool dont_quit = false; // user specified -q switch to not quit
    
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
    
    //  Initialize OpenGL bindings
    lua_State *L = m_wxlState.GetLuaState();
    //luaopen_luagl(L);
    //luaopen_luaglu(L);
    lua_register_string_ext(L);
    
    //  Initialize "LuaApp" as a shortcut to wx.wxGetApp()
    //  LuaApp.config is defined as an empty table (which may be overwritten by conf.lua)
    m_wxlState.RunString(wxT("LuaApp = wx.wxGetApp(); LuaApp.config = {}"));
    
    //  Replace package.path and package.cpath (to prevent looking outside this app)
    wxString rpath = FindResourcePath();
    wxString lpath = wxT("./?.lua;") +
        rpath + wxFILE_SEP_PATH + wxT("scripts/?.lua;") +
        rpath + wxFILE_SEP_PATH + wxT("lib/?.lua");
    wxString ext = DyLibExtension();
    wxString cpath = wxT("./?.lua;") +
        rpath + wxFILE_SEP_PATH + wxT("scripts/?.") + ext + wxT(";") +
        rpath + wxFILE_SEP_PATH + wxT("lib/?.") + ext;

    m_wxlState.RunString(wxT("package.path = ") + MakeLuaString(lpath));
    m_wxlState.RunString(wxT("package.cpath = ") + MakeLuaString(cpath));

    //  If configuration script is present in the working directory, then run it first
    wxString conf = rpath + wxFILE_SEP_PATH + wxT("scripts/conf.lua");
    if (wxFileExists(conf)) {
        int rc = m_wxlState.RunFile(conf);
        run_ok = (rc == 0);
    }
    
    //  Start console
    m_want_console = true;
    sConsoleFrame = ConsoleFrame::CreateConsoleFrame(NULL);
    sConsoleFrame->Show(true);
    SetTopWindow(sConsoleFrame);

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
    wxString files;
    int i;
    lua_newtable(L);  //  Create a global table "arg"
    for (i = 1; i < argc; i++) {
        lua_pushstring(L, argv[i]);
        lua_rawseti(L, -2, i);
        files.append(argv[i]);
        files.append(wxT("\n"));
    }
    lua_setglobal(L, "arg");

#if defined(__WXMAC__)
    //  In case of MacOS, files are basically opened via Apple Event.
    //  This line is only for exceptional case in which the executable is
    //  directly invoked from Terminal.
    if (argc > 1)
        RequestOpenFilesByEvent(files);
#else
    RequestOpenFilesByEvent(files);
#endif
    
    gInitCompleted = true;
    
    return true;
}

#if __WXMSW__
bool wxLuaStandaloneApp::CheckInstance()
{
    //  Check if the same application is already running
    char *buf, *p;
    wxString name = wxT("MyApp-") + wxGetUserId();
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
    // If acting as a debuggee, we're done - disconnect from the debugger.
//    if (m_wxlDebugTarget != NULL)
//    {
//        m_wxlDebugTarget->Stop();
//        delete m_wxlDebugTarget;
//        m_wxlDebugTarget = NULL;
//    }
    
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
        if (windows[i] != wxDynamicCast(sConsoleFrame, wxWindow)) {
            if (!windows[i]->Close())
                return;  /*  Cannot close this window  */
        }
    }
    //  Pass 3: close LuaConsole
    sConsoleFrame->Close();
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
//    if (m_pendingFilesToOpen.Count() == 0)
//        return;
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
    int i, size;
    int hideConsole = 0;
    wxFileName dname(FindResourcePath(), wxT("scripts"));
    dname.MakeAbsolute();
    wxString dpath = dname.GetFullPath();
    lua_State *L = m_wxlState.GetLuaState();
    //  Is LuaApp.config.scriptDir defined?
    m_wxlState.RunString(wxT("return LuaApp.config and LuaApp.config.scriptDir"), wxT(""), 1);
    if (!lua_isnil(L, -1)) {
        dpath = wxString(lua_tolstring(L, -1, NULL));
    }
    lua_pop(L, 1);
    size = m_pendingFilesToOpen.Count();
    if (m_numberOfProcessedFiles < 0) {
        //  First invocation: look for the lua script from the file list
        for (i = 0; i < size; i++) {
            wxFileName dname1(m_pendingFilesToOpen[i]);
            dname1.MakeAbsolute();
            wxString dpath1 = dname1.GetFullPath();
            if (wxFileName::DirExists(dpath1)) {
                wxString path1 = dpath1 + wxFILE_SEP_PATH + wxT("wxmain.lua");
                if (wxFileName::FileExists(path1)) {
                    //  This is the name of the startup script
                    //  Remove this from the file list
                    dpath = dpath1;
                    m_pendingFilesToOpen.RemoveAt(i, 1);
                    size--;
                    break;
                }
            }
        }
        wxString spath = dpath + wxFILE_SEP_PATH + wxT("wxmain.lua");
        if (wxFileExists(spath)) {
            //  Set the working directory
            wxSetWorkingDirectory(dpath);
            //  Set the package search directory
            wxString lpath = dpath + wxFILE_SEP_PATH + wxT("?.lua;");
            wxString cpath = dpath + wxFILE_SEP_PATH + wxT("?.") + DyLibExtension() + wxT(";");
            m_wxlState.RunString(wxT("package.path = ") + MakeLuaString(lpath) + wxT(".. package.path"));
            m_wxlState.RunString(wxT("package.cpath = ") + MakeLuaString(cpath) + wxT(".. package.cpath"));

            //  Load "wxmain.lua"
            DisplayMessage(wxT("Running wxmain.lua from " + dpath), false);
            int rc = m_wxlState.RunFile(spath);
            if (rc == 0) {
                if (!CheckLuaLogicalExpression(wxT("LuaApp.config.showConsole"))) {
                    hideConsole = 1;
                }
            } else {
                DisplayMessage(wxlua_LUA_ERR_msg(rc), false);
            }
        } else {
            DisplayMessage(wxT("Cannot find wxmain.lua in ") + dpath, false);
            /*  In this case, other arguments are discarded, and wait until a directory containing
                wxmain.lua is given  */
            m_numberOfProcessedFiles = -1;
            m_pendingFilesToOpen.Clear();
            return false;
        }
        m_numberOfProcessedFiles = 0;
    }
    if (size == 0) {
        //  New file: call LuaApp.NewDocument if defined
        if (CheckLuaLogicalExpression(wxT("LuaApp.NewDocument"))) {
            m_wxlState.RunString(wxT("LuaApp.NewDocument()"));
        }
    } else {
        //  Open given files
        if (CheckLuaLogicalExpression(wxT("LuaApp.OpenDocument"))) {
            while (m_pendingFilesToOpen.Count() > 0) {
                wxString file = m_pendingFilesToOpen[0];
                m_pendingFilesToOpen.RemoveAt(0);
                //  Open file: call LuaApp.OpenDocument if defined
                m_wxlState.RunString(wxT("LuaApp.OpenDocument(") + MakeLuaString(file) + wxT(")"));
                i++;
            }
        }
    }
    int count = wxTopLevelWindows.GetCount();
    if (count > 1 && hideConsole) {
        if (hideConsole) {
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
    int i, size;
    wxString files;
    size = fileNames.size();
    for (i = 0; i < size; i++) {
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
/*    wchar_t *wpath = fullpath.wc_str();
    struct _stat fileStat;
    mode_t newmode;
    if(_wstat(wpath, &fileStat) < 0)
        return;
    newmode = fileStat.st_mode | S_IXUSR | S_IXGRP | S_IXOTH;
    chmod(cpath, newmode);
*/
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
                //  TODO: Copy src to dst
                wxString src1 = src + wxFILE_SEP_PATH + name;
                wxString dst1 = dst + wxFILE_SEP_PATH + name;
                CopyRecursive(src1, dst1, allowOverwrite);
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
    lua_pop(L, 2);
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
    //  Rewrite Info.plist
    wxString plistPath = appDstPath + wxT("/Contents/Info.plist");
    wxFile plist(plistPath);
    wxString plistContent;
    plist.ReadAll(&plistContent);
    plist.Close();
    plistContent.Replace(wxT("wxLuaApp"), appName, true);
    plistContent.Replace(wxT("wxlualogo.icns"), appName + wxT(".icns"), true);
    wxFile plistOut(plistPath, wxFile::write);
    plistOut.Write(plistContent);
    plistOut.Close();
    //  Rename executable
    wxString exeName = appDstPath + wxT("/Contents/MacOS/wxLuaApp");
    wxString exeNewName = appDstPath + wxT("/Contents/MacOS/") + appName;
    ::wxRenameFile(exeName, exeNewName);
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
