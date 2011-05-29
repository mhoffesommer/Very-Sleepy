/*=====================================================================
profilergui.cpp
---------------
File created by ClassTemplate on Sun Mar 13 18:16:34 2005

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
#include <stdafx.h>
#include "profilergui.h"

#include "threadpicker.h"
#include "capturewin.h"
#include "mainwin.h"
#include "../profiler/profilerthread.h"
#include "../utils/stringutils.h"
#include "../utils/osutils.h"

// DE: 20090325 Linking fails in debug target under visual studio 2005
// RJM: works for me :-/
//#include <wx/apptrait.h>
//#if wxUSE_STACKWALKER && defined( __WXDEBUG__ )
//// silly workaround for the link error with debug configuration:
//// \src\common\appbase.cpp
//wxString wxAppTraitsBase::GetAssertStackTrace()
//{
//   return wxT("");
//}
//#endif

static const wxCmdLineEntryDesc g_cmdLineDesc[] =
{
	{ wxCMD_LINE_SWITCH, "h", "", "Displays help on the command line parameters.",			wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
	{ wxCMD_LINE_OPTION, "r", "", "Runs an executable and profiles it.",					wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL|wxCMD_LINE_NEEDS_SEPARATOR },
	{ wxCMD_LINE_OPTION, "i", "", "Loads an existing profile from a file.",					wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL|wxCMD_LINE_NEEDS_SEPARATOR },
	{ wxCMD_LINE_OPTION, "o", "", "Saves the captured profile to the given file.",			wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL|wxCMD_LINE_NEEDS_SEPARATOR },
	{ wxCMD_LINE_OPTION, "t", "", "Stops capturing automatically after N seconds time.",	wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL },
	{ wxCMD_LINE_SWITCH, "q", "", "Quiet mode (no error messages will be shown).",			wxCMD_LINE_VAL_NONE },
	{ wxCMD_LINE_SWITCH, "p", "", "Start with sampling initially paused.",					wxCMD_LINE_VAL_NONE },
	{ wxCMD_LINE_PARAM, NULL, NULL, "Loads an existing profile from a file.",				wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL},

	{ wxCMD_LINE_NONE }
};

wxIcon sleepy_icon;
std::string cmdline_load, cmdline_save;
long cmdline_timeout = 0;
std::vector<std::string> tmp_files;

ProfilerGUI::ProfilerGUI()
{

}


ProfilerGUI::~ProfilerGUI()
{

}


IMPLEMENT_APP(ProfilerGUI)

void CleanupTempFiles()
{
	for each(std::string s in tmp_files)
	{
		DeleteFile(s.c_str());
	}
	tmp_files.clear();
}

// ----------------------------------------------------------------------------
// AboutDlg
// Less-rubbish about dialog
// ----------------------------------------------------------------------------
class AboutDlg : public wxDialog
{
public:
	AboutDlg(const wxAboutDialogInfo& info);

protected:
	void AddControl(wxWindow *win, const wxSizerFlags& flags);
	void AddControl(wxWindow *win);
	void AddText(const wxString& text);

	wxSizer *m_sizerText;
};

AboutDlg::AboutDlg(const wxAboutDialogInfo& info)
{
	Init();

	if ( !wxDialog::Create(NULL, wxID_ANY, _("About ") + info.GetName(),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE) )
		return;

	m_sizerText = new wxBoxSizer(wxVERTICAL);

	wxString nameAndVersion = info.GetName();
	if ( info.HasVersion() )
		nameAndVersion << _T(' ') << info.GetVersion();

	wxSizer *sizerIconAndTitle = new wxBoxSizer(wxHORIZONTAL);
	wxIcon icon = info.GetIcon();
	if ( icon.Ok() )
	{
		sizerIconAndTitle->Add(new wxStaticBitmap(this, wxID_ANY, icon), 0, wxRIGHT, 20);
	}

	wxSizer *sizerTitle = new wxBoxSizer(wxVERTICAL);
	{
		wxStaticText *label = new wxStaticText(this, wxID_ANY, nameAndVersion);
		wxFont font(*wxNORMAL_FONT);
		font.SetPointSize(font.GetPointSize() + 3);
		font.SetWeight(wxFONTWEIGHT_BOLD);
		label->SetFont(font);

		sizerTitle->Add(label, 0, wxBOTTOM, 4);
	}
	{
		wxStaticText *label = new wxStaticText(this, wxID_ANY, info.GetDescription());
		label->SetFont(*wxITALIC_FONT);

		sizerTitle->Add(label, 0, wxBOTTOM, 4);
	}
	sizerTitle->Add(new wxStaticLine(this), wxSizerFlags().Expand());

	sizerIconAndTitle->Add(sizerTitle, 1, wxEXPAND);

	m_sizerText->Add(sizerIconAndTitle, 1, wxEXPAND);
	m_sizerText->AddSpacer(10);

	AddText("Written by:");
    for (size_t n=0;n<info.GetDevelopers().size();n++)
    {
		AddText(wxString("     »   ") + info.GetDevelopers()[n]);
    }

	AddControl(new wxStaticLine(this), wxSizerFlags().Expand());
	AddText(info.GetCopyright());

	if ( info.HasWebSite() )
	{
		AddControl(new wxStaticLine(this), wxSizerFlags().Expand());
		AddControl(new wxHyperlinkCtrl(this, wxID_ANY,
			info.GetWebSiteDescription(),
			info.GetWebSiteURL()), wxSizerFlags().Center());
	}

	wxSizer *sizerBtns = CreateButtonSizer(wxOK);
	if ( sizerBtns )
	{
		m_sizerText->Add(sizerBtns, wxSizerFlags().Expand().Border());
	}

	wxSizer *sizerTop = new wxBoxSizer(wxVERTICAL);
	sizerTop->Add(m_sizerText, 1, wxEXPAND|wxALL, 10);
	SetSizerAndFit(sizerTop);

	CentreOnScreen();
}

void AboutDlg::AddControl(wxWindow *win, const wxSizerFlags& flags)
{
	wxSizerFlags newflags = flags;
	m_sizerText->Add(win, newflags.Border(wxTOP|wxBOTTOM));
}

void AboutDlg::AddControl(wxWindow *win)
{
	AddControl(win, wxSizerFlags());
}

void AboutDlg::AddText(const wxString& text)
{
	if ( !text.empty() )
		AddControl(new wxStaticText(this, wxID_ANY, text));
}

void ProfilerGUI::ShowAboutBox()
{
	wxAboutDialogInfo info;
	info.SetName(_T("Very Sleepy"));
	info.SetVersion(VERSION);
	info.SetDescription("Open-source CPU profiler");
	info.AddDeveloper("Richard Mitton (maintainer)");
	info.AddDeveloper("Nicholas Chapman");
	info.AddDeveloper("Dan Engelbrecht");
	info.AddDeveloper("Johan Köhler");
	info.SetCopyright(_T(
		"© 2005-2010    Nicholas Chapman, Richard Mitton, and additional authors listed above.\n"
		"\n"
		"All rights reserved.\n"
		"\n"
		"This program is released under the GNU Public License.\n"
		"See LICENSE.TXT for more information."
		));
	info.SetWebSite(_T("http://www.codersnotes.com/sleepy"), _T("Very Sleepy web site"));

	AboutDlg dlg(info);
	dlg.ShowModal();
}

wxString ProfilerGUI::PromptOpen(wxWindow *parent)
{
	wxFileDialog dlg(parent, "Open File", "", "", "Sleepy Profiles (*.sleepy)|*.sleepy", 
		wxFD_OPEN);
	if (dlg.ShowModal() != wxID_CANCEL)
		return dlg.GetPath();
	else
		return wxEmptyString;
}

bool ProfilerGUI::LaunchProfiler(const AttachInfo *info, std::string &output_filename)
{
	//------------------------------------------------------------------------
	//create the profiler thread
	//------------------------------------------------------------------------
	// DE: 20090325 attaches to specific a list of threads
	ProfilerThread* profilerthread = new ProfilerThread( 
		info->process_handle,
		info->thread_handles
		);


	//------------------------------------------------------------------------
	//start the profiler thread
	//------------------------------------------------------------------------
	bool aborted = false;
	{
		CaptureWin *captureWin = new CaptureWin;
		captureWin->Show();
		captureWin->Update();

		wxStopWatch timer;
		timer.Start();

		// Make sure the thread's aren't suspended.
		if ( info->suspended )
		{
			for each( HANDLE h in info->thread_handles )	
				ResumeThread( h );
		}

		profilerthread->launch(false, THREAD_PRIORITY_TIME_CRITICAL);

		while(captureWin->UpdateProgress(profilerthread->getSampleProgress(), profilerthread->getNumThreadsRunning()))
		{
			Sleep(100);
			if (profilerthread->getNumThreadsRunning() <= 0)
				break;

			if (cmdline_timeout > 0 && timer.Time() >= cmdline_timeout*1000)
				break;
		}
		aborted = captureWin->Cancelled();
		delete captureWin;
	}

	profilerthread->commit_suicide = true;

	{
		wxProgressDialog dlg("Sleepy", "Please wait while symbols are queried...", 100);
		while(true)
		{
			int percent = profilerthread->getSymbolsPercent();
			if (percent >= 100 || profilerthread->getFailed())
				break;
			WaitForSingleObject(profilerthread->getHandle(), 100);
		}
		WaitForSingleObject(profilerthread->getHandle(), INFINITE);
	}

	bool failed = profilerthread->getFailed();
	output_filename = profilerthread->getFilename();

	delete profilerthread;
	profilerthread = NULL;

	if (failed)
		return false;

	if (aborted)
		return false;

	if (output_filename.empty())
	{
		wxLogError("There was a problem creating the profile data.");
		return false;
	}

	tmp_files.push_back(output_filename);
	atexit(CleanupTempFiles);

	return true;
}

AttachInfo::AttachInfo()
{
	suspended = false;
	process_handle = NULL;
}

AttachInfo::~AttachInfo()
{
}

AttachInfo *ProfilerGUI::RunProcess(std::string run_cmd, std::string run_cwd)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory( &si, sizeof(si) );
	ZeroMemory( &pi, sizeof(pi) );
	si.cb = sizeof(si);

	if ( !CreateProcess( NULL, (LPSTR)run_cmd.c_str(), NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, run_cwd.empty()?NULL:run_cwd.c_str(), &si, &pi ) )
	{
		wxLogSysError( "Unable to launch process" );
		return NULL;
	}
	else
	{
		AttachInfo *output = new AttachInfo;
		output->process_handle = pi.hProcess;
		output->thread_handles.push_back(pi.hThread);
		output->suspended = true;
		return output;
	}
}

bool ProfilerGUI::LoadProfileData(const std::string &filename)
{
	wxConfig config(APPNAME, VENDORNAME);

	Database *database = new Database();
	if ( !database->loadFromPath(filename,config.Read("MainWinCollapseOS",1)!=0) )
		return false;

	MainWin *frame = new MainWin(_T("Sleepy"), filename, database);

	frame->Show(TRUE);
	frame->Update();
	frame->Reset();
	return true;
}

std::string ProfilerGUI::ObtainProfileData()
{
	if (!cmdline_load.empty())
		return cmdline_load;

	AttachInfo *info;
	bool ok;
	std::string tmp_filename;

try_again:
	ThreadPicker *threadpicker = new ThreadPicker;
	int mode = threadpicker->ShowModal();
	wxLog::FlushActive();

	std::string open_filename = threadpicker->open_filename;
	std::string run_filename = threadpicker->run_filename;
	std::string run_cwd = threadpicker->run_cwd;
	info = threadpicker->attach_info;
	delete threadpicker;

	switch(mode)
	{
	case ThreadPicker::QUIT:
		return "";

	case ThreadPicker::OPEN:
		return open_filename;

	case ThreadPicker::ATTACH:
		ok = LaunchProfiler(info, tmp_filename);
		break;

	case ThreadPicker::RUN:
		info = RunProcess(run_filename,run_cwd);
		if (!info)
			goto try_again;

		ok = LaunchProfiler(info, tmp_filename);
		TerminateProcess(info->process_handle, 0);
		delete info;
		break;
	}

	if (!ok)
		return "";

	return tmp_filename;
}

bool ProfilerGUI::OnInit()
{
	::doStringUtilsUnitTests();

	wxInitAllImageHandlers();
	EnableDebugPrivilege();

	sleepy_icon = wxICON(sleepy);

	if (!wxApp::OnInit())
		return false;

	std::string filename = ObtainProfileData();
	if (filename.empty())
		return false;

	if (!cmdline_save.empty())
	{
		if (!CopyFile(filename.c_str(), cmdline_save.c_str(), FALSE))
		{
			wxLogSysError("Could not save profile data.");
			return false;
		}

		return false;
	}

	if (!LoadProfileData(filename))
		return false;

	return true;
}

void ProfilerGUI::OnInitCmdLine(wxCmdLineParser& parser)
{
	parser.DisableLongOptions();
	parser.SetDesc(g_cmdLineDesc);
	parser.SetSwitchChars("/");
}

bool ProfilerGUI::OnCmdLineParsed(wxCmdLineParser& parser)
{
	wxString param;
	std::string run_cmd, tmp_filename;

	if (parser.Found("q"))
		wxLog::EnableLogging(false);
	ProfilerThread::setPaused(parser.Found("p"));

	if (parser.Found("r") && parser.Found("i"))
	{
		parser.Usage();
		return false;
	}

	if (parser.Found("i", &param))
		cmdline_load = param.c_str();
	if (parser.GetParamCount())
		cmdline_load = parser.GetParam(0);
	if (parser.Found("o", &param))
		cmdline_save = param.c_str();
	if (!parser.Found("t", &cmdline_timeout))
		cmdline_timeout = 0;

	if (parser.Found("r", &param))
	{
		run_cmd = param.c_str();

		AttachInfo *info = RunProcess(run_cmd,"");
		if ( !info )
			return false;

		bool ok = LaunchProfiler(info, tmp_filename);
		TerminateProcess(info->process_handle, 0);
		delete info;

		if (!ok)
			return false;

		cmdline_load = tmp_filename;
	}

	return true;
}
