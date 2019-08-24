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
    
    Bind(LUAAPP_EVENT, &wxLuaStandaloneApp::OnOpenFilesByEvent, this, LuaAppEvent_openFiles);
    
    //  For wxLuaEvent, Bind() cannot be used, probably because wxLuaEvent has
    //  one more member (m_wxlState) than wxCommandEvent. Connect() can take care of it
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
    
    //  Set working directory
    wxString dirname = FindResourcePath();
    if (!dirname.IsEmpty()) {
        dirname += wxFILE_SEP_PATH;
        dirname += wxT("scripts");
        wxSetWorkingDirectory(dirname);
    }

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
    
    //  If configuration script is present in the working directory, then run it first
    wxString conf = wxT("conf.lua");
    if (wxFileExists(conf)) {
        int rc = m_wxlState.RunFile(conf);
        run_ok = (rc == 0);
    }
    
    //  Start console
    m_want_console = true;
    sConsoleFrame = ConsoleFrame::CreateConsoleFrame(NULL);
    sConsoleFrame->Show(true);
    SetTopWindow(sConsoleFrame);

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
    free(buf);
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
    }
}

void wxLuaStandaloneApp::OnLua( wxLuaEvent &event )
{
    DisplayMessage(event.GetString(), event.GetEventType() == wxEVT_LUA_ERROR);
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
            wxFileName file(m_pendingFilesToOpen[i]);
            if (file.DirExists()) {
                wxFileName name(m_pendingFilesToOpen[i], wxT("wxmain.lua"));
                if (name.Exists()) {
                    //  This is the name of the startup script
                    //  Remove this from the file list
                    dname = file;
                    dname.MakeAbsolute();
                    dpath = dname.GetFullPath();
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
            lua_getglobal(L, "package");  // [package]
            lua_pushstring(L, dpath.utf8_str()); // [package, dpath]
            char sep[8] = " ?.lua;";
            sep[0] = wxFILE_SEP_PATH;
            lua_pushstring(L, sep);       // [package, dpath, "/?.lua;"]
            lua_concat(L, 2);             // [package, "dpath/?.lua;"]
            lua_getfield(L, -2, "path");  // [package, "dpath/?.lua;", path]
            lua_concat(L, 2);             // [package, "dpath/?.lua;path"]
            lua_setfield(L, -2, "path");  // Replace package.path
            lua_pop(L, 1);                // Pop 'package' reference
            //  Load "wxmain.lua"
            DisplayMessage(wxT("Running wxmain.lua from " + dpath), false);
            int rc = m_wxlState.RunFile(spath);
            if (rc != 0) {
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
