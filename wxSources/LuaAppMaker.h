/////////////////////////////////////////////////////////////////////////////
// Purpose:     Standalone wxLua application
// Author:      John Labenski, Francis Irving, J. Winwood
// Created:     16/01/2002
// Copyright:   (c) 2012 John Labenski
// Copyright:   (c) 2002 Creature Labs. All rights reserved.
// Copyright:   (c) 2002 Lomtick Software. All rights reserved.
// Licence:     wxWidgets licence
/////////////////////////////////////////////////////////////////////////////

#ifndef LUAAPPMAKER_H
#define LUAAPPMAKER_H

#include <wx/app.h>
#include <wx/process.h>

#include "wxlua/debugger/wxldtarg.h"
#include "wxlua/wxlstate.h"
#include "wxlua/wxlconsole.h"

#if defined(__WXMSW__)

#include "wx/ipc.h"
#include "wx/snglinst.h"

#define MYAPP_IPC_TOPIC wxT("MYAPP_IPC_TOPIC")

extern wxString *gIPCServiceName;

class MyClientConnection: public wxConnection
{
public:
    virtual bool OnDisconnect();
};

class MyClient: public wxClient
{
public:
    MyClient();
    ~MyClient();
    bool Disconnect();
    wxConnectionBase *OnMakeConnection();
    MyClientConnection *m_clientConnection;
};

class MyServerConnection: public wxConnection
{
public:
    virtual bool OnDisconnect();
    virtual bool OnExec(const wxString& topic, const wxString& data);
};

class MyServer: public wxServer
{
public:
    MyServer();
    ~MyServer();
    bool Disconnect();
    wxConnectionBase *OnAcceptConnection(const wxString& topic);
    MyServerConnection *m_serverConnection;
};

#endif // defined(__WXMSW__)

// ----------------------------------------------------------------------------
// wxLuaStandaloneApp
// ----------------------------------------------------------------------------

#define MyID_CREATE_APP  1000

class wxLuaStandaloneApp : public wxApp
{
public:
    // Override the base class virtual functions
    virtual bool OnInit();
    virtual int  OnExit();

    void DisplayMessage(const wxString &msg, bool is_error);

    void OnLua(wxLuaEvent &event);

    void OnExecuteLuaScript(wxCommandEvent &event);
    void OnQuitCommand(wxCommandEvent &event);

    void RequestOpenFilesByEvent(const wxString& files);
    void OnOpenFilesByEvent(wxCommandEvent& event);
    bool OpenPendingFiles();
    bool CheckLuaLogicalExpression(wxString str);

    void OnCreateApplication(wxCommandEvent &event);

    wxString SettingsPath();
    void SaveSettings();
    void LoadSettings();

    virtual void MacOpenFiles(const wxArrayString &fileNames);
    virtual void MacNewFile();
    
    void OnOpen(wxCommandEvent &event);
    void OnAbout(wxCommandEvent &event);
    void OnUpdateUI(wxUpdateUIEvent &event);
    void OnPaintIconPanel(wxPaintEvent &event);

#if defined(__WXMSW__)
    bool CheckInstance();
#endif
    wxLuaState&         GetWxLuaState() { return m_wxlState; }

    wxString            m_programName;
    wxLuaState          m_wxlState;
    bool                m_print_stdout;
    bool                m_dos_stdout;
    bool                m_print_msgdlg;
    bool                m_want_console;
    bool                m_mem_bitmap_added;

    bool                m_wxmainExecuted;
    int           m_numberOfProcessedFiles;  // Initially -1 (startup)
    wxArrayString m_pendingFilesToOpen;  // Files to be processed by OpenPendingFiles()

#if defined(__WXMAC__)
    wxArrayString   m_filesGivenByArgv;    // Original files given by argv
#endif
    
#if defined(__WXMSW__)
public:
    wxSingleInstanceChecker *m_checker;
    wxString *m_ipcServiceName;
    MyServer *m_server;
    MyClient *m_client;
#endif

};

DECLARE_APP(wxLuaStandaloneApp)

//  A support class for wxProcess
//  When the process terminates, the exit status is kept inside the object
class wxBetterProcess : public wxProcess
{
public:
    wxBetterProcess(wxEvtHandler *parent, int id) : wxProcess(parent, id)
    {
        m_status = 0;
        m_terminated = false;
        m_killSignal = wxSIGNONE;
    }
    wxBetterProcess(int flags) : wxProcess(flags)
    {
        m_status = 0;
        m_terminated = false;
        m_killSignal = wxSIGNONE;
    }
    virtual ~wxBetterProcess() {}
    virtual void OnTerminate(int pid, int status);
    wxKillError KillProcess(wxSignal sig = wxSIGTERM, int flags = wxKILL_NOCHILDREN);
    int PutLine(wxString str);
    int GetLine(wxString &outStr);
    int GetErrorLine(wxString &outStr);
    void CloseOutput();
    bool IsTerminated() { return m_terminated; }
    int GetStatus() { return m_status; }
    int GetKillSignal() { return m_killSignal; }
protected:
    bool m_terminated;
    int m_status;
    int m_killSignal;
    wxMemoryBuffer m_stdout;
    wxMemoryBuffer m_stderr;
    wxMemoryBuffer m_stdin;
};

extern const char *gLastBuildString;
extern const char *gVersionString;

#endif // LUAAPPMAKER_H
