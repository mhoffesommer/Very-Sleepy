/*=====================================================================
profilerthread.cpp
------------------
File created by ClassTemplate on Thu Feb 24 19:29:41 2005

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
#include "../wxprofilergui/profilergui.h"
#include "profilerthread.h"

#include "../utils/stringutils.h"
#include <Psapi.h>

#pragma comment(lib, "winmm.lib")

volatile bool	ProfilerThread::m_paused;

void startProfiling(int num_samples)
{
	//get handle of the current thread
	/*const HANDLE thisthread = getProcessWideHandle();//OpenThread(THREAD_ALL_ACCESS, 0, GetCurrentThreadId());

	//------------------------------------------------------------------------
	//create profiler thread
	//------------------------------------------------------------------------
	ProfilerThread* profilerthread = new ProfilerThread(
	thisthread, //thread to target(this thread)
	num_samples, //do N samples
	false);//don't kill target when completed

	//------------------------------------------------------------------------
	//start the thread
	//------------------------------------------------------------------------
	profilerthread->launch();*/
}


const HANDLE getProcessWideHandle()
{
	HANDLE processWideThreadHandle;

	BOOL result = DuplicateHandle(
		GetCurrentProcess(),
		GetCurrentThread(),
		GetCurrentProcess(),
		&processWideThreadHandle,
		0,
		FALSE, // not inheritable
		DUPLICATE_SAME_ACCESS);

	assert(result);
	return processWideThreadHandle;
}




// DE: 20090325: Profiler has a list of threads to profile
ProfilerThread::ProfilerThread(HANDLE target_process_, const std::vector<HANDLE>& target_threads)
:	profilers(),
	target_process(target_process_)
{
	// DE: 20090325: Profiler has a list of threads to profile, one Profiler instance per thread
	profilers.reserve(target_threads.size());
	for(std::vector<HANDLE>::const_iterator it = target_threads.begin(); it != target_threads.end(); ++it){
		profilers.push_back(Profiler(target_process_, *it, callstacks, flatcounts));
	}
	numsamplessofar = 0;
	failed = false;
	symbolsPercent = 0;
	numThreadsRunning = (int)target_threads.size();

	filename = wxFileName::CreateTempFileName(wxEmptyString);
}


ProfilerThread::~ProfilerThread()
{

}


void ProfilerThread::sample(SAMPLE_TYPE timeSpent)
{
	// DE: 20090325: Profiler has a list of threads to profile, one Profiler instance per thread
	// RJM- We traverse them in random order. The act of profiling causes the Windows scheduler
	//      to re-schedule, and if we did them in sequence, it'll always schedule the first one.
	//      This starves the other N-1 threads. For lack of a better option, using a shuffle
	//      at least re-schedules them evenly.

	size_t count = profilers.size();
	if ( count == 0)
		return;

	size_t *order = (size_t *)alloca( count * sizeof(size_t) );
	for (size_t n=0;n<count;n++)
		order[n] = n;
	for (size_t n=count;n--;)
	{
		size_t i = rand() * count / (RAND_MAX+1);
		assert( i < count );
		std::swap( order[i], order[n] );
	}

	int numSuccessful = 0;
	for (size_t n=0;n<count;n++)
	{
		Profiler& profiler = profilers[order[n]];
		try {
			if (profiler.sampleTarget(timeSpent))
			{
				numsamplessofar++;
				numSuccessful++;
			}
		} catch( ProfilerExcep &e )
		{
			error("ProfilerExcep: " + e.what());
			this->commit_suicide = true;
		}
	}
				
	numThreadsRunning = numSuccessful;
}

class ProcPred
{
public:
	bool operator () (std::pair<std::string, int>& a, std::pair<std::string, int>& b)
	{
		return a.second > b.second;
	}
};

void ProfilerThread::sampleLoop()
{
	LARGE_INTEGER prev, now, freq;

	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&prev);
	
	while(!this->commit_suicide)
	{
		Sleep(m_paused?1:0);

		QueryPerformanceCounter(&now);

		__int64 diff = now.QuadPart - prev.QuadPart;
		double t = (double)diff / (double)freq.QuadPart;

		if (!m_paused)
		{
			sample(t);
		}
		
		prev = now;
	}
}

void ProfilerThread::saveData()
{
	std::cout << "computing and saving profile results..." << std::endl;

	int numsymbols = 0;
	std::map<PROFILER_ADDR, int> symbols;
	std::map<std::string, int> symbolidtable;

	//get process id of the process the target thread is running in
	//const DWORD process_id = GetProcessIdOfThread(profiler.getTarget());

	wxFFileOutputStream out(filename);
	wxZipOutputStream zip(out);
	wxTextOutputStream txt(zip);

	if (!out.IsOk() || !zip.IsOk())
	{
		error("Error writing to file");
		return;
	}

	//------------------------------------------------------------------------
	//save instruction pointer count results
	//------------------------------------------------------------------------

	char tmp[4096];

	zip.PutNextEntry(_T("Stats.txt"));

	GetModuleFileNameEx(target_process, NULL, tmp, 4096);
	time_t rawtime;
	time(&rawtime);
	txt << "Filename: " << tmp << "\n";
	txt << "Duration: " << duration << "\n";
	txt << "Date: " << asctime(localtime(&rawtime));
	txt << "Samples: " << numsamplessofar << "\n";



	zip.PutNextEntry(_T("IPCounts.txt"));

	SAMPLE_TYPE totalCounts = 0;
	for(std::map<PROFILER_ADDR, SAMPLE_TYPE>::const_iterator i = flatcounts.begin(); 
		i != flatcounts.end(); ++i)
	{
		totalCounts += i->second;
	}

	txt << totalCounts << "\n";

	for(std::map<PROFILER_ADDR, SAMPLE_TYPE>::const_iterator i = flatcounts.begin(); 
		i != flatcounts.end(); ++i)
	{
		PROFILER_ADDR addr = i->first;

		std::string addr_file;
		int addr_line;
		sym_info.getLineForAddr(addr, addr_file, addr_line);

		SAMPLE_TYPE count = i->second;

		txt << ::toHexString(addr) << " " << count << 
			"         \"" << addr_file << "\" " << addr_line << "\n";
	}

	zip.PutNextEntry(_T("Symbols.txt"));

	// Build up addr->procedure symbol table.
	std::map<PROFILER_ADDR, bool> used_addresses;
	for(std::map<PROFILER_ADDR, SAMPLE_TYPE>::const_iterator i = flatcounts.begin(); 
		i != flatcounts.end(); ++i)
	{
		PROFILER_ADDR addr = i->first;
		used_addresses[addr] = true;
	}

	for(std::map<CallStack, SAMPLE_TYPE>::const_iterator i = callstacks.begin(); 
		i != callstacks.end(); ++i)
	{
		const CallStack &callstack = i->first;
		for (size_t n=0;n<callstack.depth;n++)
		{
			used_addresses[callstack.addr[n]] = true;
		}
	}

	int done = 0;
	int used_total = static_cast<int>(used_addresses.size());
	for (std::map<PROFILER_ADDR, bool>::iterator i = used_addresses.begin(); i != used_addresses.end(); i++)
	{
		int proclinenum;
		std::string procfile;
		PROFILER_ADDR addr = i->first;

		const std::string proc_name = "\"" + sym_info.getProcForAddr(addr, procfile, proclinenum) + "\"";
		std::string full_proc_name = "\"";
		full_proc_name+= sym_info.getModuleNameForAddr(addr);
		full_proc_name+= "\" ";
		full_proc_name+= proc_name + " \"" + procfile + "\"" + " " + ::toString(proclinenum);

		if (symbolidtable.find(full_proc_name) == symbolidtable.end())
		{
			txt << full_proc_name << "\n";
			symbolidtable[full_proc_name] = numsymbols++;
		}

		symbols[addr] = symbolidtable[full_proc_name];

		symbolsPercent = MulDiv(done, 100, used_total);
		done++;
	}

	//------------------------------------------------------------------------
	//write callstack counts to disk
	//------------------------------------------------------------------------
	zip.PutNextEntry(_T("Callstacks.txt"));

	for(std::map<CallStack, SAMPLE_TYPE>::const_iterator i = callstacks.begin(); 
		i != callstacks.end(); ++i)
	{
		const CallStack &callstack = i->first;
		SAMPLE_TYPE count = i->second;

		txt << count;
		for( size_t d=0;d<callstack.depth;d++ )
		{
			txt << " " << symbols[callstack.addr[d]];
		}
		txt << "\n";
	}

	zip.PutNextEntry(_T("Version " FILE_VERSION " required"));
	txt << FILE_VERSION << "\n";


	if (!out.IsOk() || !zip.IsOk())
	{
		error("Error writing to file");
		return;
	}
}

void ProfilerThread::run()
{
	//------------------------------------------------------------------------
	//load up the debug info for it
	//
	//there's a potential problem here - if a module is unloaded after this,
	//we won't know about it. likewise, modules could be loaded at the same
	//address at different points in time. perhaps ideally we may need to
	//hook LoadLibrary/FreeLibrary calls? I suspect it's not much of a problem
	//in practice though. (bear in mind we can't load symbols once the process
	//has exited, so we have to do it up front...)
	//------------------------------------------------------------------------
	try
	{
		sym_info.loadSymbols(target_process);
	} catch(SymbolInfoExcep& e) {
		error("SymbolInfoExcep: " + e.what());
		return;
	}	

	DWORD startTick = GetTickCount();

	try
	{
		sampleLoop();
	} catch(ProfilerExcep& e) {
		// see if it's an actual error, or did the thread just finish naturally
		for(std::vector<Profiler>::const_iterator it = profilers.begin(); it != profilers.end(); ++it)
		{
			const Profiler& profiler(*it);
			if (!profiler.targetExited())
			{
				error("ProfilerExcep: " + e.what());
				return;
			}
		}

		numThreadsRunning = 0;
	}

	DWORD endTick = GetTickCount();
	int diff = endTick - startTick;
	duration = diff / 1000.0;

	saveData();

	symbolsPercent = 100;

	std::cout << "Profiling successfully completed." << std::endl;
	//::MessageBox(NULL, "Profiling successfully completed.", "Profiler", MB_OK);
}


void ProfilerThread::error(const std::string& what)
{
	failed = true;
	std::cerr << "ProfilerThread Error: " << what << std::endl;

	::MessageBox(NULL, std::string("Error: " + what).c_str(), "Profiler Error", MB_OK);
}
