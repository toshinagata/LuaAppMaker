/*
 *  ProgressDialog.h
 *
 *  Created by Toshi Nagata on 09/07/15.
 *  Copyright 2008-2021 Toshi Nagata. All rights reserved.
 *
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation version 2 of the License.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 */

#ifndef __ProgressDialog_h__
#define __ProgressDialog_h__

#include "wx/dialog.h"
#include "wx/gauge.h"
#include "wx/stattext.h"

class ProgressDialog: public wxDialog
{

public:
	ProgressDialog(const wxString title, const wxString mes);
	virtual ~ProgressDialog();
	
	void SetProgressMessage(const wxString mes);
	void SetProgressValue(double value);
	void SetInterruptValue(int value);
	int CheckInterrupt();

	wxStaticText *m_messageText;
	wxGauge *m_progressGauge;
	double m_value;
	int m_interruptValue;

private:
	DECLARE_EVENT_TABLE()
};

#endif /* __ProgressDialog_h__ */
