/*=====================================================================
launchdlg.cpp
----------------

Copyright (C) Johan Kohler

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
#include "CallstackView.h"
#include "contextmenu.h"

enum {
	ID_COLLAPSE_FUNC=2001,
	ID_COLLAPSE_MOD,
};

class FunctionMenuWindow: public wxWindow
{
public:
	int option;
	FunctionMenuWindow(wxWindow *parent) : wxWindow(parent,-1), option(0) {}
	DECLARE_EVENT_TABLE()
	void OnMenu(wxCommandEvent& event)
	{
		option = event.GetId();
	}
};


BEGIN_EVENT_TABLE(FunctionMenuWindow, wxWindow)
EVT_MENU(ID_COLLAPSE_FUNC, FunctionMenuWindow::OnMenu)
EVT_MENU(ID_COLLAPSE_MOD, FunctionMenuWindow::OnMenu)
END_EVENT_TABLE()

void FunctionMenu(wxWindow *window, wxString function, wxString mod)
{
	FunctionMenuWindow funcWindow(window);
	wxMenu *menu = new wxMenu;

	mod.UpperCase();

	menu->AppendCheckItem(ID_COLLAPSE_FUNC,"Collapse child calls")->Check(IsOsFunction(function));
	menu->AppendCheckItem(ID_COLLAPSE_MOD,wxString::Format("Collapse all %s calls", mod))->Check(IsOsModule(mod));

	funcWindow.PopupMenu(menu);
	switch(funcWindow.option) {
	case ID_COLLAPSE_FUNC:
		if (IsOsFunction(function))
			RemoveOsFunction(function);
		else
			AddOsFunction(function);
		break;
	case ID_COLLAPSE_MOD:
		if (IsOsModule(mod))
			RemoveOsModule(mod);
		else
			AddOsModule(mod);
		break;
	}
}

