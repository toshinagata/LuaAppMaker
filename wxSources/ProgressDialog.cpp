/*
 *  ProgressFrame.cpp
 *
 *  Created by Toshi Nagata on 09/07/15.
 *  Copyright 2009-2021 Toshi Nagata. All rights reserved.
 *
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation version 2 of the License.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 */

#include "ProgressDialog.h"

#include "wx/stattext.h"
#include "wx/gauge.h"
#include "wx/sizer.h"
#include "wx/evtloop.h"
#include "wx/app.h"

BEGIN_EVENT_TABLE(ProgressDialog, wxDialog)
END_EVENT_TABLE()

ProgressDialog::ProgressDialog(const wxString title, const wxString mes):
	wxDialog(NULL, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxCAPTION)
{
	//  Vertical sizer containing (1) message text, (2) progress gauge, (3) note text
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
	
	m_messageText = new wxStaticText(this, -1, wxT("Message"), wxDefaultPosition, wxSize(240, 40), wxST_NO_AUTORESIZE);
	sizer->Add(m_messageText, 0, wxALL | wxEXPAND, 10);   // Can expand horizontally
	
	m_progressGauge = new wxGauge(this, -1, 10000, wxDefaultPosition, wxSize(240, 24), wxGA_HORIZONTAL);
	sizer->Add(m_progressGauge, 0, wxALL | wxEXPAND, 10);
	
	wxStaticText *noteText = new wxStaticText(this, -1, wxT("Press ESC to interrupt"), wxDefaultPosition, wxSize(240, 20), wxALIGN_CENTRE | wxST_NO_AUTORESIZE);

    noteText->SetFont(*wxSMALL_FONT);
	sizer->Add(noteText, 0, wxALL | wxEXPAND, 10);

	m_value = -1.0;
	m_progressGauge->Pulse();
	m_messageText->SetLabel(mes);
	
	m_interruptValue = 0;

	sizer->Layout();
	this->SetSizerAndFit(sizer);
	this->Centre();
	this->Show();
	
}

ProgressDialog::~ProgressDialog()
{
}

void
ProgressDialog::SetProgressMessage(const wxString mes)
{
	m_messageText->SetLabel(mes);
	if (m_value < 0)
		m_progressGauge->Pulse();
	Update();
}

void
ProgressDialog::SetProgressValue(double value)
{
	m_value = value;
	if (value < 0)
		m_progressGauge->Pulse();
	else
		m_progressGauge->SetValue((int)(value * 10000));
	Update();
}

void
ProgressDialog::SetInterruptValue(int value)
{
	m_interruptValue = value;
}

int
ProgressDialog::CheckInterrupt()
{
	if (m_interruptValue) {
		int save = m_interruptValue;
		m_interruptValue = 0;
		return save;
	}

    wxTheApp->SafeYieldFor(this, wxEVT_CATEGORY_UI);
    
/*	wxEventLoopBase * const loop = wxEventLoopBase::GetActive();
	if (loop != NULL)
		loop->YieldFor(wxEVT_CATEGORY_UI); */

    if (::wxGetKeyState(WXK_ESCAPE))
		return 1;
	else return 0;
}
