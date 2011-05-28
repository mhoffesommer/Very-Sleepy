/*=====================================================================
symbolinfo.h
------------
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
#ifndef __SYMBOLINFO_H_666_
#define __SYMBOLINFO_H_666_


#include <string>
#include <windows.h>
#include <vector>
#include "profiler.h"

class Module
{
public:
	Module(PROFILER_ADDR base_addr_, const std::string& name_) { base_addr = base_addr_; name = name_; }
	PROFILER_ADDR base_addr;
	std::string name;
};

class SymbolInfoExcep
{
public:
	SymbolInfoExcep(const std::string& s_) : s(s_) {}
	~SymbolInfoExcep(){}	

	const std::string& what() const { return s; }
private:
	std::string s;
};

/*=====================================================================
SymbolInfo
----------
Wrapper around some DbgHelp API stuff.
=====================================================================*/
class SymbolInfo
{
public:
	/*=====================================================================
	SymbolInfo
	----------
	
	=====================================================================*/
	SymbolInfo();

	~SymbolInfo();

	void loadSymbols(HANDLE process_handle);//throws SymbolInfoExcep

	const std::string getModuleNameForAddr(PROFILER_ADDR addr);

	//get proc name for mem address
	const std::string getProcForAddr(PROFILER_ADDR addr, std::string& procfilepath_out, int& proclinenum_out);


	void getLineForAddr(PROFILER_ADDR addr, std::string& filepath_out, int& linenum_out);


	// For internal use.
	void addModule(const Module& module);
	// For internal use. Sort modules by increasing base address.
	void sortModules();
	HANDLE process_handle;

private:
	std::vector<Module> modules;
	bool is64BitProcess;
};



#endif //__SYMBOLINFO_H_666_




