/*=====================================================================
profiler.cpp
------------
File created by ClassTemplate on Thu Feb 24 19:00:30 2005

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
#include "profiler.h"


#include "../utils/stringutils.h"
#include "../utils/osutils.h"
#include <process.h>
#include <iostream>
#include <assert.h>
#include <winnt.h>
#include <dbghelp.h>

// DE: 20090325: Profiler no longer owns callstack and flatcounts since it is shared between multipler profilers

Profiler::Profiler(HANDLE target_process_, HANDLE target_thread_, std::map<CallStack, SAMPLE_TYPE>& callstacks_, std::map<PROFILER_ADDR, SAMPLE_TYPE>& flatcounts_)
:	target_process(target_process_),
	target_thread(target_thread_),
	callstacks(callstacks_),
	flatcounts(flatcounts_)
{
	is64BitProcess = Is64BitProcess(target_process);
}

// DE: 20090325: Need copy constructor since it is put in a std::vector

Profiler::Profiler(const Profiler& iOther)
:	target_process(iOther.target_process),
	target_thread(iOther.target_thread),
	callstacks(iOther.callstacks),
	flatcounts(iOther.flatcounts),
	is64BitProcess(iOther.is64BitProcess)
{
}

// DE: 20090325: Need copy assignement since it is put in a std::vector

Profiler& Profiler::operator=(const Profiler& iOther)
{
	target_process = iOther.target_process;
	target_thread = iOther.target_thread;
	callstacks = iOther.callstacks;
	flatcounts = iOther.flatcounts;

	return *this;
}

Profiler::~Profiler()
{
	
}

#ifdef _WIN64

bool Profiler::sampleTargetWoW(SAMPLE_TYPE timeSpent)
{
	CallStack stack;
	stack.depth = 0;

	STACKFRAME64 frame;
	memset(&frame, 0, sizeof(frame));

	WOW64_CONTEXT threadcontext;
	threadcontext.ContextFlags = WOW64_CONTEXT_i386 | WOW64_CONTEXT_CONTROL;

	DWORD result = fn_Wow64SuspendThread(target_thread);
	if(result == 0xffffffff)
		return false;

	int prev_priority = GetThreadPriority(target_thread);

	SetThreadPriority(target_thread, THREAD_PRIORITY_TIME_CRITICAL);
	result = fn_Wow64GetThreadContext(target_thread, &threadcontext);
	SetThreadPriority(target_thread, prev_priority);

	if(!result){
		DWORD error = GetLastError();
		// DE: 20090325: If GetThreadContext fails we must be sure to resume thread again
		ResumeThread(target_thread);
		throw ProfilerExcep("GetThreadContext failed.");
	}

	stack.addr[stack.depth++] = threadcontext.Eip;
	frame.AddrStack.Offset = threadcontext.Esp;
	frame.AddrPC.Offset = threadcontext.Eip;
	frame.AddrFrame.Offset = threadcontext.Ebp;
	frame.AddrStack.Mode = frame.AddrPC.Mode = frame.AddrFrame.Mode = AddrModeFlat;
	
	while(true)
	{
		BOOL result = StackWalk64(
			IMAGE_FILE_MACHINE_I386,
			target_process,
			target_thread,
			&frame,
			&threadcontext,
			NULL,
			&SymFunctionTableAccess64,
			&SymGetModuleBase64,
			NULL
			);

		if (!result || stack.depth >= MAX_CALLSTACK_LEVELS || frame.AddrReturn.Offset == 0)
			break;

		stack.addr[stack.depth++] = (PROFILER_ADDR)frame.AddrReturn.Offset;
	}

	//std::cout << "addr: " << addr << std::endl;

	result = ResumeThread(target_thread);
	if(!result)
		throw ProfilerExcep("ResumeThread failed.");

	//NOTE: this has to go after ResumeThread.  Otherwise mem allocation needed by std::map
	//may hit a lock held by the suspended thread.

	flatcounts[stack.addr[0]]+=timeSpent;
	callstacks[stack]+=timeSpent;
	return true;
}
#endif

bool Profiler::sampleTarget(SAMPLE_TYPE timeSpent)
{
	// DE: 20090325: Moved declaration of stack variables to reduce size of code inside Suspend/Resume thread

#if defined(_WIN64)
	if(!is64BitProcess) {
		return sampleTargetWoW(timeSpent);
	}
#endif
	CallStack stack;
	stack.depth = 0;

	STACKFRAME64 frame;
	memset(&frame, 0, sizeof(frame));

	CONTEXT threadcontext;

#if defined(_WIN64)
	threadcontext.ContextFlags = CONTEXT_AMD64 | CONTEXT_CONTROL;
#else
	threadcontext.ContextFlags = CONTEXT_i386 | CONTEXT_CONTROL;
#endif
	// An open question is whether or not this routine can be called
	// re-entrantly by the multi-media timer support.

	// Can fail occasionally, for example if you have a debugger attached to the process.
	HRESULT result = SuspendThread(target_thread);
	if(result == 0xffffffff)
		return false;

	int prev_priority = GetThreadPriority(target_thread);
	
	SetThreadPriority(target_thread, THREAD_PRIORITY_TIME_CRITICAL);
	result = GetThreadContext(target_thread, &threadcontext);
	SetThreadPriority(target_thread, prev_priority);


	if(!result){
		// DE: 20090325: If GetThreadContext fails we must be sure to resume thread again
		ResumeThread(target_thread);
		throw ProfilerExcep("GetThreadContext failed.");
	}

#if defined(_WIN64)
	stack.addr[stack.depth++] = threadcontext.Rip;
	frame.AddrStack.Offset = threadcontext.Rsp;
	frame.AddrPC.Offset = threadcontext.Rip;
	frame.AddrFrame.Offset = threadcontext.Rbp;
	frame.AddrStack.Mode = frame.AddrPC.Mode = frame.AddrFrame.Mode = AddrModeFlat;
#else
	stack.addr[stack.depth++] = threadcontext.Eip;
	frame.AddrStack.Offset = threadcontext.Esp;
	frame.AddrPC.Offset = threadcontext.Eip;
	frame.AddrFrame.Offset = threadcontext.Ebp;
	frame.AddrStack.Mode = frame.AddrPC.Mode = frame.AddrFrame.Mode = AddrModeFlat;
#endif
	while(true)
	{
		BOOL result = StackWalk64(
#if defined(_WIN64)
			IMAGE_FILE_MACHINE_AMD64,
#else
			IMAGE_FILE_MACHINE_I386,
#endif
			target_process,
			target_thread,
			&frame,
			&threadcontext,
			NULL,
			&SymFunctionTableAccess64,
			&SymGetModuleBase64,
			NULL
		);

		if (!result || stack.depth >= MAX_CALLSTACK_LEVELS || frame.AddrReturn.Offset == 0)
			break;

		stack.addr[stack.depth++] = (PROFILER_ADDR)frame.AddrReturn.Offset;
	}

	//std::cout << "addr: " << addr << std::endl;

	result = ResumeThread(target_thread);
	if(!result)
		throw ProfilerExcep("ResumeThread failed.");

	//NOTE: this has to go after ResumeThread.  Otherwise mem allocation needed by std::map
	//may hit a lock held by the suspended thread.

	flatcounts[stack.addr[0]]+=timeSpent;
	callstacks[stack]+=timeSpent;
	return true;
}

// returns true if the target thread has finished
bool Profiler::targetExited() const
{
	DWORD code = WaitForSingleObject(target_thread, 0);
	return (code == WAIT_OBJECT_0);
}


//void Profiler::saveIPs(std::ostream& stream)
//{
//	for(std::map<Sample, int>::const_iterator i = counts.begin(); 
//		i != counts.end(); ++i)
//	{
//		const Sample &sample = i->first;
//		int count = i->second;
//		stream << ::toHexString(sample.addr) << " " << count << "\n";
//	}
//
//	stream.flush();
//}
//
