/*=====================================================================
profilergui.h
-------------
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
#ifndef __PROFILERGUI_H_666_
#define __PROFILERGUI_H_666_

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/notebook.h>
#include <wx/aui/aui.h>
#include <wx/progdlg.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/txtstrm.h>
#include <wx/aboutdlg.h>
#include <wx/cmdline.h>
#include <wx/statline.h>
#include <wx/hyperlink.h>
#include <wx/evtloop.h>
#include <wx/tipwin.h>
#include <string>
#include <vector>
#include <map>

#define VERSION			"0.7"
#define APPNAME			"Very Sleepy"
#define VENDORNAME		"codersnotes.com"

extern wxIcon sleepy_icon;

class SymbolInfo;

struct AttachInfo
{
	AttachInfo();
	~AttachInfo();

	HANDLE process_handle;
	std::vector<HANDLE> thread_handles;
	bool suspended;
};

/*=====================================================================
ProfilerGUI
-----------
the main app
=====================================================================*/
class ProfilerGUI : public wxApp
{
public:
	ProfilerGUI();
	virtual ~ProfilerGUI();
	virtual bool OnInit();

	static void ShowAboutBox();
	static wxString PromptOpen(wxWindow *parent);

protected:
	virtual void OnInitCmdLine(wxCmdLineParser& parser);
	virtual bool OnCmdLineParsed(wxCmdLineParser& parser);

	bool LaunchProfiler(const AttachInfo *info, std::string &output_filename);
	AttachInfo *RunProcess(std::string run_cmd,std::string run_cwd);
	bool LoadProfileData(const std::string &filename);
	std::string ObtainProfileData();
};

DECLARE_APP(ProfilerGUI)

#endif //__PROFILERGUI_H_666_




