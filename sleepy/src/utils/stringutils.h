/*=====================================================================
stringutils.cpp
---------------

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

http://www.gnu.org/copyleft/gpl.html.
=====================================================================*/
#ifndef __STRINGUTILS_H__
#define __STRINGUTILS_H__


//NOTE: not all of this code has been used/tested for ages.
//Your mileage may vary; test the code before you use it!!!!



#pragma warning(disable:4786)
//disable long name warning


inline float stringToFloat(const std::string& s)
{
	return (float)atof(s.c_str());
}

inline int stringToInt(const std::string& s)
{
	return atoi(s.c_str());
}

inline double stringToDouble(const std::string& s)
{
	return atof(s.c_str());
}


unsigned int hexStringToUInt(const std::string& s);
unsigned long long hexStringTo64UInt(const std::string& s);

//const std::string toHexString(unsigned int i);//32 bit integers
const std::string toHexString(unsigned long long i);//for 64 bit integers
const std::string intToString(int i);
const std::string floatToString(float f);
const std::string doubleToString(double d);
const std::string floatToString(float f, int num_decimal_places);



//argument overloaded toString functions:
inline const std::string toString(double f)
{
	return doubleToString(f);
}

inline const std::string toString(float f)
{
	return floatToString(f);
}

inline const std::string toString(int i)
{
	return intToString(i);
}

inline const std::string toString(char c)
{
	return std::string(1, c);
}





__forceinline bool isWhitespace(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

__forceinline bool isAlpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

__forceinline bool isCToken(char c)
{
	return isAlpha(c) || c == '_' || c == '#';
}

inline void concatWithChar(std::string& s, char c)
{
	s.resize(s.size() + 1);
	s[s.size() - 1] = c;
}

void readQuote(std::istream& stream, std::string& str_out);//reads string from between double quotes.


//NOTE: must be in debug mode for this to work
void doStringUtilsUnitTests();

struct StringSet
{
	std::vector<std::string>	strings;
	bool caseCheck;
public:
	StringSet(const char *file, bool caseCheck);
	void Add(const char *string);
	void Remove(const char *string);
	bool Contains(const char *string) const;
};



#endif //__STRINGUTILS_H__