/*=====================================================================
capturewin.cpp
----------------

Copyright (C) Richard Mitton

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

http://www.gnu.org/copyleft/gpl.html.
=====================================================================*/
#include "capturewin.h"

BEGIN_EVENT_TABLE(CaptureWin, wxDialog)
EVT_BUTTON(wxID_OK, CaptureWin::OnOk)
EVT_BUTTON(wxID_CANCEL, CaptureWin::OnCancel)
END_EVENT_TABLE()

CaptureWin::CaptureWin()
:	wxDialog(NULL, -1, wxString(_T("Sleepy")), 
			 wxDefaultPosition, wxDefaultSize,
			 wxDEFAULT_DIALOG_STYLE)
{
	cancelled = stopped = false;

	wxBoxSizer *rootsizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *panelsizer = new wxBoxSizer(wxVERTICAL);

	wxPanel *panel = new wxPanel(this);

	wxStaticText *text1 = new wxStaticText( panel, -1, "Profiling application..." );
	wxStaticText *text2 = new wxStaticText( panel, -1, "Press OK to stop profiling and display collected results." );
	progressText = new wxStaticText( panel, -1, "-" );
	progressBar = new wxGauge( panel, -1, 0, wxDefaultPosition, wxSize(100,18) );

	int border = ConvertDialogToPixels(wxSize(2, 0)).x;
	wxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
	buttons->AddStretchSpacer();
	buttons->Add(new wxButton(panel, wxID_OK),					0, wxALIGN_RIGHT | wxLEFT|wxRIGHT,	border);
	buttons->Add(new wxButton(panel, wxID_CANCEL),				0, wxALIGN_RIGHT | wxLEFT,			border);

	panelsizer->Add(text1, 0, wxBOTTOM, 4);
	panelsizer->Add(text2, 0, wxBOTTOM, 10);
	panelsizer->Add(progressText, 0, wxBOTTOM, 5);
	panelsizer->Add(progressBar, 0, wxBOTTOM|wxEXPAND, 10);
	panelsizer->Add(buttons, 0, wxEXPAND);

	panel->SetSizer(panelsizer);
	panel->SetAutoLayout(TRUE);

	rootsizer->Add(panel, 1, wxEXPAND | wxALL, 10);
	SetSizer(rootsizer);
	rootsizer->SetSizeHints(this);
	SetAutoLayout(TRUE);

	SetSize(wxSize(300, 160));
	Centre();
}

CaptureWin::~CaptureWin()
{
}

bool CaptureWin::UpdateProgress(int numSamples, int numThreads)
{
	char tmp[256];
	sprintf(tmp, "%i samples, %.1fs elapsed, %i threads running", numSamples, stopwatch.Time() / 1000.0f, numThreads);
	progressText->SetLabel(tmp);

	progressBar->Pulse();

	Update();
	wxYieldIfNeeded();
	
	return !stopped;
}

void CaptureWin::OnOk(wxCommandEvent& event)
{
	stopped = true;
}

void CaptureWin::OnCancel(wxCommandEvent& event)
{
	cancelled = true;
	stopped = true;
}
