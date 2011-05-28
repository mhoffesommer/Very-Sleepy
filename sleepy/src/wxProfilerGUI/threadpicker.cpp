/*=====================================================================
threadpicker.cpp
----------------
File created by ClassTemplate on Sun Mar 20 17:12:56 2005

Copyright (C) Nicholas Chapman

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
#include "threadpicker.h"
#include "launchdlg.h"
#include "../profiler/symbolinfo.h"
#include <dbghelp.h>

// IDs for the controls and the menu commands
enum
{
	// menu items
	ProcWin_Exit = 1,
	ProcWin_Refresh,
	ProcWin_Launch,

	// it is important for the id corresponding to the "About" command to have
	// this standard value as otherwise it won't be handled properly under Mac
	// (where it is special and put into the "Apple" menu)
	ProcWin_About = wxID_ABOUT
};

BEGIN_EVENT_TABLE(ThreadPicker, wxModalFrame)
EVT_BUTTON(wxID_OK, ThreadPicker::OnAttachProfiler)
EVT_BUTTON(wxID_SELECTALL, ThreadPicker::OnAttachProfilerAll)
EVT_MENU(wxID_OPEN, ThreadPicker::OnOpen)
EVT_LIST_ITEM_ACTIVATED(PROCESS_LIST, ThreadPicker::OnDoubleClicked)
EVT_MENU(ProcWin_Exit, ThreadPicker::OnQuit)
EVT_MENU(ProcWin_Refresh, ThreadPicker::OnRefresh)
EVT_MENU(ProcWin_Launch, ThreadPicker::OnLaunchExe)
EVT_MENU(ProcWin_About, ThreadPicker::OnAbout)	
EVT_BUTTON(ProcWin_Refresh, ThreadPicker::OnRefresh)
EVT_CLOSE(ThreadPicker::OnClose)
EVT_BUTTON(ProcWin_Exit, ThreadPicker::OnQuit)
END_EVENT_TABLE()

ThreadPicker::ThreadPicker()
:	wxModalFrame(NULL, -1, wxString(_T("Sleepy")), 
			 wxDefaultPosition, wxDefaultSize,
			 wxDEFAULT_FRAME_STYLE)
{
    SetIcon(sleepy_icon);

	wxMenu *menuFile = new wxMenu;

	// the "About" item should be in the help menu
	wxMenu *helpMenu = new wxMenu;
	helpMenu->Append(ProcWin_About, _T("&About...\tF1"), _T("Show about dialog"));

	menuFile->Append(wxID_OPEN, _T("&Open..."), _T("Opens an existing profile"));
	menuFile->Append(ProcWin_Launch, _T("&Launch..."), _T("Launches a new executable to profile"));
	menuFile->Append(ProcWin_Refresh, _T("&Refresh"), _T("Refreshes the process list"));
	menuFile->AppendSeparator();
	menuFile->Append(ProcWin_Exit, _T("E&xit\tAlt-X"), _T("Quit this program"));

	// now append the freshly created menu to the menu bar...
	wxMenuBar *menuBar = new wxMenuBar();
	menuBar->Append(menuFile, _T("&File"));
	menuBar->Append(helpMenu, _T("&Help"));

	// ... and attach this menu bar to the frame
	SetMenuBar(menuBar);

	wxBoxSizer *rootsizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *panelsizer = new wxBoxSizer(wxVERTICAL);

	wxPanel *panel = new wxPanel(this);
	wxButton *ok_button = new wxButton(panel, wxID_OK, "Profile &Selected");
	wxButton *all_button = new wxButton(panel, wxID_SELECTALL,"Profile &All");
	all_button->Disable();
	all_button->SetDefault();

	// DE: 20090325 one list for processes and one list for selected process threads
	threadlist = new ThreadList(panel, wxDefaultPosition, wxDefaultSize, ok_button, all_button);
	processlist = new ProcessList(panel, wxDefaultPosition, wxDefaultSize, threadlist);

	rootsizer->Add(panel, 1, wxEXPAND | wxALL);
	panelsizer->Add(new wxStaticText(panel, -1, "Select a process to profile:"), 0, wxLEFT|wxRIGHT|wxTOP, 10);
	panelsizer->Add(processlist, 1, wxEXPAND | wxALL, 10);

	// DE: 20090325 title for thread list
	panelsizer->Add(new wxStaticText(panel, -1, "Select thread(s) to profile: (CTRL-click for multiple)"), 0, wxLEFT|wxRIGHT, 10);
	panelsizer->Add(threadlist, 1, wxEXPAND | wxALL, 10);

	int border = ConvertDialogToPixels(wxSize(2, 0)).x;
	wxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
	buttons->Add(new wxButton(panel, ProcWin_Refresh, "Refresh"),	0, wxALIGN_LEFT  | wxRIGHT,			border);
	buttons->AddStretchSpacer();
	buttons->Add(all_button,										0, wxALIGN_RIGHT | wxLEFT|wxRIGHT,	border);
	buttons->Add(ok_button,											0, wxALIGN_RIGHT | wxLEFT|wxRIGHT,	border);
	//buttons->Add(new wxButton(panel, ProcWin_Exit,	"E&xit"),		0, wxALIGN_RIGHT | wxLEFT,			border);

	panelsizer->Add(buttons, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 10);

	panel->SetSizer(panelsizer);
	panel->SetAutoLayout(TRUE);

	SetSizer(rootsizer);
	rootsizer->SetSizeHints(this);
	SetAutoLayout(TRUE);

	SetSize(wxSize(400, 500));
	Centre();
}

void ThreadPicker::OnOpen(wxCommandEvent& event)
{
	open_filename = ProfilerGUI::PromptOpen(this);
	if (!open_filename.empty())
	{
		EndModal(OPEN);
	}
}

void ThreadPicker::OnAttachProfiler()
{
	if ( AttachToProcess(false) )
	{
		EndModal(ATTACH);
	}
}

void ThreadPicker::OnAttachProfiler(wxCommandEvent& event)
{
	OnAttachProfiler();
}

void ThreadPicker::OnAttachProfilerAll(wxCommandEvent& event)
{
	if ( AttachToProcess(true) )
	{
		EndModal(ATTACH);
	}
}

void ThreadPicker::OnDoubleClicked(wxListEvent& event)
{
	OnAttachProfiler();
}

void ThreadPicker::OnClose(wxCloseEvent& event)
{
	EndModal(QUIT);
}

void ThreadPicker::OnQuit(wxCommandEvent& event)
{
	EndModal(QUIT);
}

void ThreadPicker::OnRefresh(wxCommandEvent& event)
{
	processlist->updateProcesses();
}

void ThreadPicker::OnLaunchExe(wxCommandEvent& event)
{
	wxConfig config(APPNAME, VENDORNAME);

	wxString prevCmdPath;
	config.Read("PrevLaunchPath", &prevCmdPath, "");
	wxString prevCwd;
	config.Read("PrevLaunchCwd", &prevCwd, "");

	LaunchDlg dlg(this);
	dlg.SetCmdValue(prevCmdPath);
	dlg.SetCwdValue(prevCwd);
	if (dlg.ShowModal() != wxID_OK)
		return;

	run_filename = dlg.GetCmdValue();
	config.Write("PrevLaunchPath", run_filename);
	run_cwd = dlg.GetCwdValue();
	config.Write("PrevLaunchCwd", run_cwd);
	EndModal(RUN);
}

void ThreadPicker::OnAbout(wxCommandEvent& event)
{
	ProfilerGUI::ShowAboutBox();
}


ThreadPicker::~ThreadPicker()
{

}

/*
unsigned int ThreadPicker::getSelectedThread()
{
	const ProcessInfo* info = processlist->getSelectedProcess();

	if(info)
	{
		return info->threads[0].getID();
	}

	return 0;
}
*/

bool ThreadPicker::AttachToProcess(bool allThreads)
{
	attach_info = new AttachInfo;

	const ProcessInfo* processInfo = processlist->getSelectedProcess();
	if(processInfo == NULL)
	{
		wxLogError("No process selected.");
		return false;
	}
	else
	{
		wxConfig config(APPNAME, VENDORNAME);
		config.Write("PrevProcess",processInfo->getName());
	}

	//------------------------------------------------------------------------
	//Get handle to target process
	//------------------------------------------------------------------------
	attach_info->process_handle = processInfo->getProcessHandle(); 
	
	// Check it didn't exit.
	if (WaitForSingleObject(attach_info->process_handle, 0) == WAIT_OBJECT_0)
		attach_info->process_handle = NULL;

	if ( attach_info->process_handle == NULL )
	{
		wxLogError("Cannot attach to running process.");
		return false;
	}

	// DE: 20090325 attaches to specific a list of threads
	std::vector<const ThreadInfo*> selectedThreads = threadlist->getSelectedThreads(allThreads);
	if (selectedThreads.size() == 0)
	{
		selectedThreads = threadlist->getSelectedThreads(true);
	}
	if (selectedThreads.size() == 0)
	{
		wxLogError("No thread(s) selected.");
		return false;
	}

	// DE: 20090325 attaches to specific a list of threads
	for(std::vector<const ThreadInfo*>::const_iterator it = selectedThreads.begin(); it != selectedThreads.end(); ++it)
	{
		const ThreadInfo* threadInfo(*it);

		HANDLE threadHandle = threadInfo->getThreadHandle();
		

		if (threadHandle == NULL)
		{
			DWORD err = GetLastError();
			wxLogError("Cannot attach to selected thread.");
		} else {
			attach_info->thread_handles.push_back(threadHandle);
		}
	}

	// DE: 20090325 attaches to specific a list of threads
	if (attach_info->thread_handles.size() == 0){
		wxLogError("Cannot attach to any threads.");
		return false;
	}

	return true;
}

