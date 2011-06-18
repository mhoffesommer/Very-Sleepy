// Simple API to control Very Sleepy profiler.
// Copyright (C) 2011 Martin Hoffesommer, http://hoffesommer.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
// 
// http://www.gnu.org/copyleft/gpl.html..

#include <stdafx.h>
#include "apiserver.h"
#include "profiler/profilerthread.h"

class ApiServer::Connection: public wxDDEConnection
{
public:
	virtual bool OnExecute(const wxString&, char* data, int size, wxIPCFormat format)
	{
		if (!strncmp(data,"pause ",6))
		{
			ProfilerThread::setPaused(data[6]!='0');
		}
		else
		{
			return false;
		}

		return true;
	}

	virtual char* OnRequest(const wxString&, const wxString& item, int *size, wxIPCFormat fmt)
	{
		if (item=="paused")
		{
			return ProfilerThread::isPaused()?"1":"0";
		}
		else
		{
			return NULL;
		}
	}
};

bool ApiServer::Create()
{
	return wxDDEServer::Create("VerySleepyProfilerServer");
}

wxConnectionBase *ApiServer::OnAcceptConnection(const wxString& topic)
{
	if (topic=="Control.1")
	{
		return new Connection;
	}
	else
	{
		return NULL;
	}
}
