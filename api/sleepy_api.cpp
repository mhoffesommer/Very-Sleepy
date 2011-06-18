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

#include "sleepy_api.h"
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddeml.h>
#include <assert.h>

//========================================================================================================
// SleepyApi::Impl

class SleepyApi::Impl
{
public:
	Impl();
	~Impl();

	bool IsAttached() const;
	void Pause(bool doPause);
	bool IsPaused() const;

private:
	Impl(const Impl&);
	Impl& operator=(const Impl&);

	static Impl	*m_instance;

	DWORD	m_appId;
	HCONV	m_conv;
	HSZ		m_service,m_topic,m_paused;

	bool TryConnect();
	static HDDEDATA CALLBACK OnDdeCallback(UINT type, UINT fmt, HCONV hconv, HSZ hsz1, HSZ hsz2, HDDEDATA hdata, ULONG_PTR data1, ULONG_PTR data2);
};

SleepyApi::Impl* SleepyApi::Impl::m_instance=NULL;

SleepyApi::Impl::Impl()
:	m_conv(NULL)
{
	assert(m_instance==NULL);
	m_instance=this;

	m_appId=0;
	if (DdeInitializeA(&m_appId,OnDdeCallback,APPCMD_CLIENTONLY,0)!=DMLERR_NO_ERROR)
	{
		m_appId=0;
	}
	else
	{
		m_service=DdeCreateStringHandleA(m_appId,"VerySleepyProfilerServer",CP_WINANSI);
		m_topic=DdeCreateStringHandleA(m_appId,"Control.1",CP_WINANSI);
		m_paused=DdeCreateStringHandleA(m_appId,"paused",CP_WINANSI);

		TryConnect();
	}
}

SleepyApi::Impl::~Impl()
{
	if (m_conv)
	{
		DdeDisconnect(m_conv);
		m_conv=NULL;
	}
	if (m_appId)
	{
		DdeFreeStringHandle(m_appId,m_service);
		DdeFreeStringHandle(m_appId,m_topic);
		DdeFreeStringHandle(m_appId,m_paused);
		
		DdeUninitialize(m_appId);
	}
	assert(m_instance==this);
	m_instance=NULL;
}

bool SleepyApi::Impl::IsAttached() const
{
	return m_conv!=NULL;
}

void SleepyApi::Impl::Pause(bool doPause)
{
	if (!m_conv)
		return;

	DdeClientTransaction((LPBYTE)(doPause?"pause 1":"pause 0"),7,m_conv,NULL,0,XTYP_EXECUTE,1000,NULL);
}

bool SleepyApi::Impl::IsPaused() const
{
	if (!m_conv)
		return false;

	HDDEDATA res=DdeClientTransaction(NULL,0,m_conv,m_paused,CF_TEXT,XTYP_REQUEST,1000,NULL);
	if (!res)
		return false;

	char help[10];
	DdeGetData(res,(LPBYTE)help,sizeof(help),0);
	DdeFreeDataHandle(res);

	return *help!='0';
}

bool SleepyApi::Impl::TryConnect()
{
	assert(!m_conv);
	m_conv=DdeConnect(m_appId,m_service,m_topic,NULL);
	return m_conv!=NULL;
}

HDDEDATA CALLBACK SleepyApi::Impl::OnDdeCallback(UINT type, UINT fmt, HCONV hconv, HSZ hsz1, HSZ hsz2, HDDEDATA hdata, ULONG_PTR data1, ULONG_PTR data2)
{
	if (!m_instance)
		return FALSE;

	if (type==XTYP_REGISTER&&!m_instance->m_conv)
	{
		char help[30];
		DdeQueryStringA(m_instance->m_appId,hsz1,help,sizeof(help),CP_WINANSI);
		if (!strcmp(help,"VerySleepyProfilerServer"))
		{
			m_instance->TryConnect();
		}
	}
	if (type==XTYP_UNREGISTER&&m_instance->m_conv)
	{
		char help[30];
		DdeQueryStringA(m_instance->m_appId,hsz1,help,sizeof(help),CP_WINANSI);
		if (!strcmp(help,"VerySleepyProfilerServer"))
		{
			DdeDisconnect(m_instance->m_conv);
			m_instance->m_conv=NULL;
		}
	}
	return FALSE;
}

//========================================================================================================
// SleepyApi

SleepyApi& SleepyApi::Instance()
{
	static SleepyApi i;
	return i;
}

SleepyApi::SleepyApi()
:	m_impl(new Impl)
{
}

SleepyApi::~SleepyApi()
{
	delete m_impl;
}

bool SleepyApi::IsAttached() const
{
	return m_impl->IsAttached();
}

void SleepyApi::Pause(bool doPause)
{
	m_impl->Pause(doPause);
}

bool SleepyApi::IsPaused() const
{
	return m_impl->IsPaused();
}
