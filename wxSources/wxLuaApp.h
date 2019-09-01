/////////////////////////////////////////////////////////////////////////////
// Purpose:     Standalone wxLua application
// Author:      John Labenski, Francis Irving, J. Winwood
// Created:     16/01/2002
// Copyright:   (c) 2012 John Labenski
// Copyright:   (c) 2002 Creature Labs. All rights reserved.
// Copyright:   (c) 2002 Lomtick Software. All rights reserved.
// Licence:     wxWidgets licence
/////////////////////////////////////////////////////////////////////////////

#ifndef WX_LUAAPP_H
#define WX_LUAAPP_H

#include <wx/app.h>

//#include "wxlua/debugger/wxldtarg.h"
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
    void Disconnect();
    wxConnectionBase *OnMakeConnection();
    MyClientConnection *m_clientConnection;
};

class MyServerConnection: public wxConnection
{
public:
    virtual bool OnDisconnect();
    virtual bool OnExecute(const wxString& topic, const void *data, size_t size, wxIPCFormat format);
};

class MyServer: public wxServer
{
public:
    MyServer();
    ~MyServer();
    void Disconnect();
    wxConnectionBase *OnAcceptConnection(const wxString& topic);
    MyServerConnection *m_serverConnection;
};

#endif // defined(__WXMSW__)

// ----------------------------------------------------------------------------
// wxLuaStandaloneApp
// ----------------------------------------------------------------------------

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

    void RequestOpenFilesByEvent(wxString& files);
    void OnOpenFilesByEvent(wxCommandEvent& event);
    bool OpenPendingFiles();
    bool CheckLuaLogicalExpression(wxString str);

    virtual void MacOpenFiles(const wxArrayString &fileNames);
    virtual void MacNewFile();
    
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
//    wxLuaDebugTarget*   m_wxlDebugTarget;

    int           m_numberOfProcessedFiles;  // Initially -1 (startup)
    wxArrayString m_pendingFilesToOpen;  // Files to be processed by OpenPendingFiles()

#if defined(__WXMSW__)
public:
    wxSingleInstanceChecker *m_checker;
    wxString *m_ipcServiceName;
    MyServer *m_server;
    MyClient *m_client;
#endif

};

DECLARE_APP(wxLuaStandaloneApp)

#endif // WX_LUAAPP_H
