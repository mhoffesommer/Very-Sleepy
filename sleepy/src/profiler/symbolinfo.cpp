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


BOOL CALLBACK EnumModules(
    PCSTR    ModuleName, 
    DWORD64 BaseOfDll,  
    PVOID   UserContext )
{
	std::cout << toHexString(BaseOfDll) << " " << ModuleName << std::endl;

	SymbolInfo* syminfo = static_cast<SymbolInfo*>(UserContext);
	syminfo->addModule(Module((PROFILER_ADDR)BaseOfDll, ModuleName));

    return TRUE;
}


SymbolInfo::SymbolInfo()
:	process_handle(NULL)
{
}

void SymbolInfo::loadSymbols(HANDLE process_handle_)
{
	process_handle = process_handle_;

	is64BitProcess = Is64BitProcess(process_handle);

	//process_handle = NULL;

	//process_handle = GetCurrentProcess();

	//use undecorated names
	//SymSetOptions(SYMOPT_UNDNAMES);

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
		if(!is64BitProcess) {
			Options |= SYMOPT_INCLUDE_32BIT_MODULES; 
		}
#endif

		SymSetOptions( Options ); 

		if(!SymInitialize(process_handle, NULL, TRUE))
		{
			DWORD error = GetLastError();
			if (error==0x8000000d)
			{
				// can happen if EXE start takes a bit longer...
				Sleep(100);
				continue;
			}
			throw SymbolInfoExcep("SymInitialize failed.");
		}

		std::cout << "symbols loaded for modules:" << std::endl;

		if(!SymEnumerateModules64(process_handle, EnumModules, this))
			throw SymbolInfoExcep("SymEnumerateModules64 failed.");

		if (!modules.empty())
			break;

		// Sometimes the module enumeration will fail (no error code, but no modules
		// will be returned). If we try again a little later it seems to work.
		// I suspect this may be if we try and enum modules too early on, before the process
		// has really had a chance to 'get going'.
		// Perhaps a better solution generally would be to manually load module symbols on demand,
		// as each sample comes in? That'd also solve the problem of modules getting loaded/unloaded
		// mid-profile. Yes, I'll probably do that some day.
		Sleep(100);
		SymCleanup(process_handle);
	}

	sortModules();
}


SymbolInfo::~SymbolInfo()
{
	//------------------------------------------------------------------------
	//clean up
	//------------------------------------------------------------------------
	if ( process_handle )
	{
		if (!SymCleanup(process_handle))
		{
			//error
		}

		process_handle = NULL;
	}
}

const std::string SymbolInfo::getModuleNameForAddr(PROFILER_ADDR addr)
{
	if(modules.empty())
		return "[unknown module]";

	if(addr < modules[0].base_addr)
		return "[unknown module]";

	for(unsigned int i=1; i<modules.size(); ++i)
		if(addr < modules[i].base_addr)
			return modules[i-1].name;

	//assign any addresses past the base of the last module to the last module.
	//NOTE: this is not strictly correct, but without the sizes of the modules, a decent way of doing things.
	return modules[modules.size() - 1].name;
}

void SymbolInfo::addModule(const Module& module)
{
	modules.push_back(module);
}

void SymbolInfo::sortModules()
{
	struct Sorter {
		bool operator() (const Module& a, const Module& b) const {
			return a.base_addr < b.base_addr;
		}
	};
	std::sort(modules.begin(), modules.end(), Sorter());
}

const std::string SymbolInfo::getProcForAddr(PROFILER_ADDR addr, 
											 std::string& procfilepath_out, int& proclinenum_out)
{
	procfilepath_out = "";
	proclinenum_out = 0;

	unsigned char buffer[1024];

	//blame MS for this abomination of a coding technique
	SYMBOL_INFO* symbol_info = (SYMBOL_INFO*)buffer;
	symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
	symbol_info->MaxNameLen = sizeof(buffer) - sizeof(SYMBOL_INFO) + 1;

	BOOL result = SymFromAddr(process_handle, (DWORD64)addr, 0, symbol_info);

	if(!result)
	{
		DWORD err = GetLastError();
		char buf[256];
#if defined(_WIN64)
		if(is64BitProcess)
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

void SymbolInfo::getLineForAddr(PROFILER_ADDR addr, std::string& filepath_out, int& linenum_out)
{
	DWORD displacement;
	IMAGEHLP_LINE64 lineinfo;
	ZeroMemory(&lineinfo, sizeof(lineinfo));
	lineinfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	BOOL result = SymGetLineFromAddr64(process_handle, (DWORD64)addr, &displacement, &lineinfo);

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



