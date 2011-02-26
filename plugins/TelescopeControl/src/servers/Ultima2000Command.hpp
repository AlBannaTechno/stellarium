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

#ifndef _ULTIMA_2000_COMMAND_HPP_
#define _ULTIMA_2000_COMMAND_HPP_

#include <QtGlobal>
#include <QTextStream>
using namespace std;

class Server;
class TelescopeClientDirectUltima2000;

//! Abstract base class for Celestron Ultima 2000 (and compatible) commands.
class Ultima2000Command
{
public:
	virtual ~Ultima2000Command() {}
	virtual bool writeCommandToBuffer(char *&buff, char *end) = 0;
	bool hasBeenWrittenToBuffer() const { return has_been_written_to_buffer; }
	virtual int readAnswerFromBuffer(const char *&buff, const char *end) const = 0;
	virtual bool needsNoAnswer() const { return false; }
	virtual void print(QTextStream &o) const = 0;
	// returns true when reading is finished
	
protected:
	Ultima2000Command(Server &server);
	TelescopeClientDirectUltima2000 &server;
	bool has_been_written_to_buffer;
};

inline QTextStream &operator<<(QTextStream &o, const Ultima2000Command &c)
{
	c.print(o);
	return o;
}

//! Celestron Ultima2000 command: Slew to a given position.
//! Sends the Celestron 16-bit "go to RA/Dec" command <tt>R34AB,12CE</tt>.
class Ultima2000CommandGotoPosition : public Ultima2000Command
{
public:
	Ultima2000CommandGotoPosition(Server &server, quint16 ra_int, qint16 dec_int);
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char *&buff, const char *end) const;
	void print(QTextStream &o) const;
	
private:
	quint16 ra;
	qint16 dec;
};

//! Celestron Ultima 2000 command: Get the current position.
//! Sends the Celestron 16-bit "get RA/Dec" command <tt>E</tt> and reads the
//! answer in format <tt>34AB,12CE#</tt>.
class Ultima2000CommandGetRaDec : public Ultima2000Command
{
public:
	Ultima2000CommandGetRaDec(Server &server) : Ultima2000Command(server) {}
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char *&buff, const char *end) const;
	void print(QTextStream &o) const;
};

#endif//_ULTIMA_2000_COMMAND_HPP_
