/*
Stellarium telescope control
Copyright (c) 2006 Johannes Gajdosik
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

#ifndef _ULTIMA_2000_CONNECTION_HPP_
#define _ULTIMA_2000_CONNECTION_HPP_

#include "SerialPortUltima2000.hpp"

#include <list>
using namespace std;

class Ultima2000Command;

//! Serial port connection to a Celestron Ultima 2000 or a compatible telescope.
class Ultima2000Connection : public SerialPortUltima2000
{
public:
	Ultima2000Connection(Server &server, const char *serial_device);
	~Ultima2000Connection() { resetCommunication(); }
	void sendGoto(unsigned int ra_int, int dec_int);
	void sendCommand(Ultima2000Command * command);
	void setTimeBetweenCommands(long long int micro_seconds)
	{
		time_between_commands = micro_seconds;
	}
	
private:
	//! Parses read buffer data received from the telescope.
	void dataReceived(const char *&p, const char *read_buff_end);
	//! Not implemented, as this is not a connection to a client.
	void sendPosition(unsigned int ra_int, int dec_int, int status) {Q_UNUSED(ra_int); Q_UNUSED(dec_int); Q_UNUSED(status);}
	void resetCommunication();
	void prepareSelectFds(fd_set &read_fds, fd_set &write_fds, int &fd_max);
	bool writeFrontCommandToBuffer();
	//! Flushes the command queue, sending commands to the write buffer.
	//! This method iterates over the queue, writing to the write buffer
	//! as many commands as possible, until it reaches a command that
	//! requires an answer.
	void flushCommandList();
	
private:
	list<Ultima2000Command*> commandQueue;
	long long int time_between_commands;
	long long int next_send_time;
	long long int read_timeout_endtime;
	int goto_commands_queued;
};

#endif //_ULTIMA_2000_CONNECTION_HPP_
