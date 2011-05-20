/*
Stellarium telescope control
Copyright (c) 2006 Johannes Gajdosik
Copyright (c) 2006 Michael Heinz (NexStar modifications)
Copyright (c) 2010 Bogdan Marinov (ServoCAT modifications)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "ServoCatCommand.hpp"
#include "TelescopeClientDirectServoCat.hpp"
#include "LogFile.hpp"

#include <cmath>

using namespace std;

ServoCatCommand::ServoCatCommand(Server &server) :
		server(*static_cast<TelescopeClientDirectServoCat*>(&server)),
		has_been_written_to_buffer(false)
{
}

ServoCatCommandGotoPosition::ServoCatCommandGotoPosition(Server &server, double ra_double, double dec_double) :
		ServoCatCommand(server)
{
	dec = dec_double;
	ra = ra_double;
}

#define UINTTOASCII(x) (((x)<10)?('0'+(x)):('0'))
#define ASCIITOUINT(x) (((x)>='0'&&(x)<='9')?((x)-'0'):0)

bool ServoCatCommandGotoPosition::writeCommandToBuffer(char *&p,char *end)
{
	//The command should be 16 bytes long
	if (end-p < 16)
		return false;

	//"Checksum"
	char checksum;

	//[1] GOTO command
	*p++ = 'g';
	
	//The middle 16 bytes are used to compute the checksum
	char ch;

	//[6] Right ascension
	//TODO: Better digit extraction?
	int x = ra * 1000;
	ch = UINTTOASCII(x/10000); x%=10000;
	checksum = ch; *p++ = ch;
	ch = UINTTOASCII(x/1000); x%=1000;
	checksum ^= ch; *p++ = ch;
	ch = '.';
	checksum ^= ch; *p++ = ch;
	ch = UINTTOASCII(x/100); x%=100;
	checksum ^= ch; *p++ = ch;
	ch = UINTTOASCII(x/10); x%=10;
	checksum ^= ch; *p++ = ch;
	ch = UINTTOASCII(x);
	checksum ^= ch; *p++ = ch;

	//[1] Separator
	ch = ' ';
	checksum ^= ch; *p++ = ch;

	//[7] Declination, with sign
	if (dec < 0)
	{
		dec = -dec;
		ch = '-';
	}
	else
		ch = '+';
	checksum ^= ch; *p++ = ch;

	x = dec * 1000;
	ch = UINTTOASCII(x/10000); x%=10000;
	checksum = ch; *p++ = ch;
	ch = UINTTOASCII(x/1000); x%=1000;
	checksum ^= ch; *p++ = ch;
	ch = '.';
	checksum ^= ch; *p++ = ch;
	ch = UINTTOASCII(x/100); x%=100;
	checksum ^= ch; *p++ = ch;
	ch = UINTTOASCII(x/10); x%=10;
	checksum ^= ch; *p++ = ch;
	ch = UINTTOASCII(x);
	checksum ^= ch; *p++ = ch;

	//[1] Checksum byte
	*p++ = checksum;

	has_been_written_to_buffer = true;
	return true;
}

int ServoCatCommandGotoPosition::readAnswerFromBuffer(const char *&buff, const char *end) const
{
	if (buff >= end)
		return 0;
	
	if (*buff=='g')
	{
		#ifdef DEBUG4
		*log_file << Now() << "ServoCatCommandGotoPosition::readAnswerFromBuffer: slew ok"
		          << endl;
		#endif
	}
	else
	{
		#ifdef DEBUG4
		*log_file << Now() << "ServoCatCommandGotoPosition::readAnswerFromBuffer: slew failed." << endl;
		#endif
	}
	buff++;
	return 1;
}

void ServoCatCommandGotoPosition::print(QTextStream &o) const
{
	o << "ServoCatCommandGotoPosition("
	  << ra << "," << dec <<')';
}

bool ServoCatCommandGetRaDec::writeCommandToBuffer(char *&p, char *end)
{
	if (end-p < 1)
		return false;

	*p++ = '\r'; //Carriage return, ASCII code 0x0D

	has_been_written_to_buffer = true;
	return true;
}

int ServoCatCommandGetRaDec::readAnswerFromBuffer(const char *&buff, const char *end) const
{
	//Skip possible answers to the GoTo command
	if (buff < end && *buff=='g')
		buff++;

	//The answer should be 16 bytes long
	if (end-buff < 16)
		return 0;

	const char *p = buff;

	//[1] Answer begins with a space
	if (*p++ != ' ')
	{
		#ifdef DEBUG4
		*log_file << Now() << "ServoCatCommandGetRaDec::readAnswerFromBuffer: "
		                      "error: '[space]' expected" << endl;
		#endif
		return -1;
	}

	//[6] Right ascension
	double ra = 0.0;
	ra += ASCIITOUINT(*p) * 10; p++;
	ra += ASCIITOUINT(*p); p++;
	if (*p++ != '.')
	{
		#ifdef DEBUG4
		*log_file << Now() << "ServoCatCommandGetRaDec::readAnswerFromBuffer: "
		                      "error: '.' expected" << endl;
		#endif
		return -1;
	}
	ra += ASCIITOUINT(*p) * 0.1; p++;
	ra += ASCIITOUINT(*p) * 0.01; p++;
	ra += ASCIITOUINT(*p) * 0.001; p++;

	//[1] Separator
	if (*p++ != ' ')
	{
		#ifdef DEBUG4
		*log_file << Now() << "ServoCatCommandGetRaDec::readAnswerFromBuffer: "
		                      "error: '[space]' expected" << endl;
		#endif
		return -1;
	}

	//[7] Declination
	//Declination sign
	if ((*p != '+') && (*p != '-'))
	{
		#ifdef DEBUG4
		*log_file << Now() << "ServoCatCommandGetRaDec::readAnswerFromBuffer: "
		                      "error: '+' or '-' expected" << endl;
		#endif
		return -1;
	}
	bool negative = (*p == '-') ? true : false;
	p++;
	//Declination value
	double dec = 0;
	dec += ASCIITOUINT(*p) * 10; p++;
	dec += ASCIITOUINT(*p); p++;
	if (*p++ != '.')
	{
		#ifdef DEBUG4
		*log_file << Now() << "ServoCatCommandGetRaDec::readAnswerFromBuffer: "
		                      "error: '.' expected" << endl;
		#endif
		return -1;
	}
	dec += ASCIITOUINT(*p) * 0.1; p++;
	dec += ASCIITOUINT(*p) * 0.01; p++;
	dec += ASCIITOUINT(*p) * 0.001; p++;
	if (negative)
		dec = -dec;

	//[1] Final character
	if (*p++ != '\0')
	{
		#ifdef DEBUG4
		*log_file << Now() << "ServoCatCommandGetRaDec::readAnswerFromBuffer: "
		                      "error: '\0' expected" << endl;
		#endif
		return -1;
	}
	
	#ifdef DEBUG4
	*log_file << Now() << "ServoCatCommandGetRaDec::readAnswerFromBuffer: "
	                      "ra = " << ra << ", dec = " << dec
	          << endl;
	#endif
	buff = p;

	//ra and dec are decimal fractions, so no sub-unit conversions like in LX200
	const unsigned int ra_int = (unsigned int)floor(ra * (4294967296.0/24.0));
	const int dec_int = (int)floor(dec * (4294967296.0/360.0));
	server.raDecReceived(ra_int, dec_int);
	return 1;
}

void ServoCatCommandGetRaDec::print(QTextStream &o) const
{
	o << "ServoCatCommandGetRaDec";
}

