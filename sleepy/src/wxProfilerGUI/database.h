/*=====================================================================
database.h
----------

Copyright (C) Nicholas Chapman
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
#ifndef __DATABASE_H_666_
#define __DATABASE_H_666_

#include "profilergui.h"
#include "lineinfo.h"
#include <deque>

bool IsOsFunction(wxString function);
void AddOsFunction(wxString function);
void RemoveOsFunction(wxString function);

bool IsOsModule(wxString mod);
void AddOsModule(wxString mod);
void RemoveOsModule(wxString mod);

/*=====================================================================
Database
--------

=====================================================================*/
class Database
{
public:
	struct Symbol
	{
		std::string module;
		std::string procname;
		std::string sourcefile;
		bool		isCollapseFunction;
		bool		isCollapseModule;
		int         sourceline;
	};

	struct Item
	{
		const Symbol *symbol;
		double inclusive, exclusive;
	};

	struct List
	{
		List() { totalcount = 0; }

		std::vector<Item> items;
		double totalcount;
	};

	struct CallStack
	{
		std::vector<const Symbol *> stack;
		double samplecount;
	};

	Database();
	virtual ~Database();
	void clear();

	bool loadFromPath(const std::string& profilepath,bool collapseOSCalls);
	bool reload(bool collapseOSCalls);
	void scanRoot();

	const List &getRoot() const { return rootList; }
	List getCallers(const Symbol *symbol) const;
	List getCallees(const Symbol *symbol) const;
	std::vector<const CallStack*> getCallstacksContaining(const Symbol *symbol) const;
	const LINEINFOMAP *getLineInfo(const std::string &srcfile) const;
	
	std::vector<std::string> stats;

	std::string getProfilePath() const { return profilepath; }

private:
	std::vector<Symbol *> symbols;
	std::deque<CallStack> callstacks;
	std::map<std::string, LINEINFOMAP > fileinfo;
	List rootList;
	std::string profilepath;

	void loadSymbols(wxInputStream &file);
	void loadProcList(wxInputStream &file,bool collapseKernelCalls);
	void loadIpCounts(wxInputStream &file);
	void loadStats(wxInputStream &file);
};

#endif //__DATABASE_H_666_
