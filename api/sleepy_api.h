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

#ifndef SLEEPY_API_H
#define SLEEPY_API_H

class SleepyApi 
{
public:
	// get the API instance
	static SleepyApi& Instance();

	// is the profiler currently attached?
	bool IsAttached() const;

	// pause/resume profiler
	void Pause(bool doPause=true);
	void Resume() { Pause(false); }

	// is the profiler currently paused?
	bool IsPaused() const;

private:
	SleepyApi();
	~SleepyApi();
	SleepyApi(const SleepyApi&);
	SleepyApi& operator=(const SleepyApi&);

	class Impl;
	Impl	*m_impl;
};

#endif
