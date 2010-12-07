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

#ifndef _SERVOCAT_COMMAND_HPP_
#define _SERVOCAT_COMMAND_HPP_

#include <QTextStream>
using namespace std;

class Server;
class TelescopeClientDirectServoCat;

//! Abstract base class for StellarCat ServoCAT (and compatible) commands.
//! The code is based on NexStarCommand, which in turn is based on Lx200Command.
class ServoCatCommand
{
public:
	virtual ~ServoCatCommand(void) {}
	virtual bool writeCommandToBuffer(char *&buff, char *end) = 0;
	bool hasBeenWrittenToBuffer(void) const { return has_been_written_to_buffer; }
	virtual int readAnswerFromBuffer(const char *&buff, const char *end) const = 0;
	virtual bool needsNoAnswer(void) const { return false; }
	virtual void print(QTextStream &o) const = 0;
	// returns true when reading is finished
	
protected:
	ServoCatCommand(Server &server);
	TelescopeClientDirectServoCat &server;
	bool has_been_written_to_buffer;
};

inline QTextStream &operator<<(QTextStream &o, const ServoCatCommand &c)
{
	c.print(o);
	return o;
}

//! Celestron ServoCat command: Slew to a given position.
class ServoCatCommandGotoPosition : public ServoCatCommand
{
public:
	ServoCatCommandGotoPosition(Server &server, double ra, double dec);
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char *&buff, const char *end) const;
	virtual bool needsNoAnswer(void) const { return true; }
	void print(QTextStream &o) const;
	
private:
	double ra, dec;
};

//! Celestron ServoCat command: Get the current position.
class ServoCatCommandGetRaDec : public ServoCatCommand
{
public:
	ServoCatCommandGetRaDec(Server &server) : ServoCatCommand(server) {}
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char *&buff, const char *end) const;
	void print(QTextStream &o) const;
};

#endif //_SERVOCAT_COMMAND_HPP_
