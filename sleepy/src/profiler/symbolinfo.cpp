/*=====================================================================
symbolinfo.cpp
--------------
File created by ClassTemplate on Sat Mar 05 19:10:20 2005

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

http://www.gnu.org/copyleft/gpl.html..
=====================================================================*/
#include <stdafx.h>
#include "symbolinfo.h"

#include "../utils/stringutils.h"
#include "../utils/osutils.h"
#include <dbghelp.h>

std::string	SymbolInfo::m_symPath;

SymbolInfo::SymbolInfo()
:	m_processHandle(NULL)
{
}

SymbolInfo::~SymbolInfo()
{
	if ( m_processHandle )
	{
		SymCleanup(m_processHandle);
	}
}

void SymbolInfo::loadSymbols(HANDLE process_handle_)
{
	m_processHandle = process_handle_;
	m_is64BitProcess = Is64BitProcess(m_processHandle);

	for( int n=0;n<4;n++ )
	{
		DWORD Options = SymGetOptions(); 

#ifdef _DEBUG
		// SYMOPT_DEBUG option asks DbgHelp to print additional troubleshooting 
		// messages to debug output - use the debugger's Debug Output window 
		// to view the messages 
		Options |= SYMOPT_DEBUG;
#endif

#ifdef _WIN64
		if(!m_is64BitProcess) 
		{
			Options |= SYMOPT_INCLUDE_32BIT_MODULES; 
		}
#endif

		SymSetOptions( Options ); 

		if (!SymInitialize(m_processHandle,m_symPath.empty()?NULL:m_symPath.c_str(),TRUE))
		{
			DWORD error = GetLastError();
			if (error==0x8000000d)
			{
				// can happen if EXE start takes a bit longer...
				Sleep(250);
				continue;
			}
			throw SymbolInfoExcep("SymInitialize failed.");
		}

		if (!SymEnumerateModules64(m_processHandle,EnumModules,this))
			throw SymbolInfoExcep("SymEnumerateModules64 failed.");

		if (!m_base2module.empty())
			break;

		// Sometimes the module enumeration will fail (no error code, but no modules
		// will be returned). If we try again a little later it seems to work.
		// I suspect this may be if we try and enum modules too early on, before the process
		// has really had a chance to 'get going'.
		// Perhaps a better solution generally would be to manually load module symbols on demand,
		// as each sample comes in? That'd also solve the problem of modules getting loaded/unloaded
		// mid-profile. Yes, I'll probably do that some day.
		Sleep(100);
		SymCleanup(m_processHandle);
	}
}

const char* SymbolInfo::getModuleNameForAddr(PROFILER_ADDR addr) const
{
	if (m_base2module.empty()||
		addr<m_base2module.begin()->first)
	{
		return "[unknown module]";
	}

	std::map<PROFILER_ADDR,std::string>::const_iterator i=m_base2module.begin();
	for (;;)
	{
		std::map<PROFILER_ADDR,std::string>::const_iterator j=i++;
		//assign any addresses past the base of the last module to the last module.
		//NOTE: this is not strictly correct, but without the sizes of the modules, a decent way of doing things.
		if (i==m_base2module.end()||
			addr<i->first)
		{
			return j->second.c_str();
		}
	}
}

BOOL CALLBACK SymbolInfo::EnumModules(PCSTR ModuleName, DWORD64 BaseOfDll, PVOID UserContext )
{
	SymbolInfo* syminfo=static_cast<SymbolInfo*>(UserContext);
	syminfo->m_base2module[(PROFILER_ADDR)BaseOfDll]=ModuleName;

	return TRUE;
}

const std::string SymbolInfo::getProcForAddr(PROFILER_ADDR addr, 
											 std::string& procfilepath_out, int& proclinenum_out) const
{
	procfilepath_out = "";
	proclinenum_out = 0;

	unsigned char buffer[1024];

	//blame MS for this abomination of a coding technique
	SYMBOL_INFO* symbol_info = (SYMBOL_INFO*)buffer;
	symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
	symbol_info->MaxNameLen = sizeof(buffer) - sizeof(SYMBOL_INFO) + 1;

	BOOL result = SymFromAddr(m_processHandle, (DWORD64)addr, 0, symbol_info);

	if(!result)
	{
		DWORD err = GetLastError();
		char buf[256];
#if defined(_WIN64)
		if(m_is64BitProcess)
			sprintf(buf, "[%016llX]", addr);
		else 
			sprintf(buf, "[%08X]", unsigned __int32(addr));
#else
		sprintf(buf, "[%08X]", addr);
#endif
		return buf;
	}

	//------------------------------------------------------------------------
	//lookup proc file and line num
	//------------------------------------------------------------------------
	getLineForAddr((PROFILER_ADDR)symbol_info->Address, procfilepath_out, proclinenum_out);

	return symbol_info->Name;
}

void SymbolInfo::getLineForAddr(PROFILER_ADDR addr, std::string& filepath_out, int& linenum_out) const
{
	DWORD displacement;
	IMAGEHLP_LINE64 lineinfo;
	ZeroMemory(&lineinfo, sizeof(lineinfo));
	lineinfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	BOOL result = SymGetLineFromAddr64(m_processHandle, (DWORD64)addr, &displacement, &lineinfo);

	if(result)
	{
		filepath_out = lineinfo.FileName;
		linenum_out = lineinfo.LineNumber;
	}
	else
	{
		DWORD err = GetLastError();
		filepath_out = "[unknown]";
		linenum_out = 0;
	}
}
