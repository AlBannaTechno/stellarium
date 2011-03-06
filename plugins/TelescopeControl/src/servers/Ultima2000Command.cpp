/*
Stellarium telescope control
Copyright (c) 2006 Johannes Gajdosik
Copyright (c) 2006 Michael Heinz (NexStar modifications)
Copyright (c) 2011 Bogdan Marinov (Ultima2000 modifications)

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

#include "Ultima2000Command.hpp"
#include "TelescopeClientDirectUltima2000.hpp"
#include "LogFile.hpp"

#include <cmath>

using namespace std;

Ultima2000Command::Ultima2000Command(Server &server) : server(*static_cast<TelescopeClientDirectUltima2000*>(&server)), has_been_written_to_buffer(false)
{
}

Ultima2000CommandGotoPosition::Ultima2000CommandGotoPosition(Server &server,
                                                             quint16 ra_int,
                                                             qint16 dec_int)
	: Ultima2000Command(server),
	ra(ra_int),
	dec(dec_int)
{
	//
}

#define NIBTOASCII(x) (((x)<10)?('0'+(x)):('A'+(x)-10))
#define ASCIITONIB(x) (((x)<'A')?((x)-'0'):((x)-'A'+10))

bool Ultima2000CommandGotoPosition::writeCommandToBuffer(char *&p,char *end)
{
	#ifdef DEBUG5
	char *b = p;
	#endif
	
	//Is there enough space in the buffer? The GOTO command is 10 bytes long.
	if (end-p < 10)
		return false;

	*p++ = 'R';
	
	//16-bit RA to hexadecimal digits
	int x = ra;
	//*p++ = NIBTOASCII ((x>>28) & 0x0f);
	//*p++ = NIBTOASCII ((x>>24) & 0x0f);
	//*p++ = NIBTOASCII ((x>>20) & 0x0f);
	//*p++ = NIBTOASCII ((x>>16) & 0x0f);
	*p++ = NIBTOASCII ((x>>12) & 0x0f); 
	*p++ = NIBTOASCII ((x>>8) & 0x0f); 
	*p++ = NIBTOASCII ((x>>4) & 0x0f); 
	*p++ = NIBTOASCII (x & 0x0f); 
	*p++ = ',';

	//16-bit Dec to hexadecimal digits
	x = dec;
	//*p++ = NIBTOASCII ((x>>28) & 0x0f);
	//*p++ = NIBTOASCII ((x>>24) & 0x0f);
	//*p++ = NIBTOASCII ((x>>20) & 0x0f);
	//*p++ = NIBTOASCII ((x>>16) & 0x0f);
	*p++ = NIBTOASCII ((x>>12) & 0x0f); 
	*p++ = NIBTOASCII ((x>>8) & 0x0f); 
	*p++ = NIBTOASCII ((x>>4) & 0x0f); 
	*p++ = NIBTOASCII (x & 0x0f); 
	*p = 0;

	has_been_written_to_buffer = true;
	#ifdef DEBUG5
	*log_file << Now() << "Ultima2000CommandGotoPosition::writeCommandToBuffer:"
	          << b << endl;
	#endif
	
	return true;
}

int Ultima2000CommandGotoPosition::readAnswerFromBuffer(const char *&buff, const char *end) const
{
	if (buff >= end)
		return 0;
	
	if (*buff=='#')
	{
		#ifdef DEBUG4
		*log_file << Now() << "Ultima2000CommandGotoPosition::readAnswerFromBuffer: slew ok"
		          << endl;
		#endif
	}
	else
	{
		#ifdef DEBUG4
		*log_file << Now() << "Ultima2000CommandGotoPosition::readAnswerFromBuffer: slew failed." << endl;
		#endif
	}
	buff++;
	return 1;
}

void Ultima2000CommandGotoPosition::print(QTextStream &o) const
{
	o << "Ultima2000CommandGotoPosition("
	  << ra << "," << dec <<')';
}

bool Ultima2000CommandGetRaDec::writeCommandToBuffer(char *&p, char *end)
{
	if (end-p < 1)
		return false;

	*p++ = 'E';
	has_been_written_to_buffer = true;
	return true;
}

int Ultima2000CommandGetRaDec::readAnswerFromBuffer(const char *&buff, const char *end) const
{
	//Is there enough space in the buffer? The answer is 10 bytes long.
	if (end-buff < 10)
		return 0;

	//Note: the parameters of TelescopeClientDirectUltima2000::raDecReceived()
	//are "unsigned int" at the moment, so something has to be changed in the
	//future.
	quint32 ra, dec;
	const char *p = buff;

	// Next 4 bytes are RA as hexadecimal digits
	ra = 0;
	ra += ASCIITONIB(*p); ra <<= 4; p++;
	ra += ASCIITONIB(*p); ra <<= 4; p++;
	ra += ASCIITONIB(*p); ra <<= 4; p++;
	//ra += ASCIITONIB(*p); ra <<= 4; p++;
	//ra += ASCIITONIB(*p); ra <<= 4; p++;
	//ra += ASCIITONIB(*p); ra <<= 4; p++;
	//ra += ASCIITONIB(*p); ra <<= 4; p++;
	ra += ASCIITONIB(*p); p++;

	if (*p++ != ',')
	{
		#ifdef DEBUG4
		*log_file << Now() << "Ultima2000CommandGetRaDec::readAnswerFromBuffer: "
		                      "error: ',' expected" << endl;
		#endif
		return -1;
	}

	// Next 4 bytes are Dec as hexadecimal digits
	dec = 0;
	dec += ASCIITONIB(*p); dec <<= 4; p++;
	dec += ASCIITONIB(*p); dec <<= 4; p++;
	dec += ASCIITONIB(*p); dec <<= 4; p++;
	//dec += ASCIITONIB(*p); dec <<= 4; p++;
	//dec += ASCIITONIB(*p); dec <<= 4; p++;
	//dec += ASCIITONIB(*p); dec <<= 4; p++;
	//dec += ASCIITONIB(*p); dec <<= 4; p++;
	dec += ASCIITONIB(*p); p++;

	if (*p++ != '#')
	{
		#ifdef DEBUG4
		*log_file << Now() << "Ultima2000CommandGetRaDec::readAnswerFromBuffer: "
		                      "error: '#' expected" << endl;
		#endif
		return -1;
	}
	
	
	#ifdef DEBUG4
	*log_file << Now() << "Ultima2000CommandGetRaDec::readAnswerFromBuffer: "
	                      "ra = " << ra << ", dec = " << dec
	          << endl;
	#endif
	buff = p;

	//Conversion back to 32-bit values
	ra *= 65536;
	dec *= 65536;
	server.raDecReceived(ra, dec);
	return 1;
}

void Ultima2000CommandGetRaDec::print(QTextStream &o) const
{
	o << "Ultima2000CommandGetRaDec";
}

