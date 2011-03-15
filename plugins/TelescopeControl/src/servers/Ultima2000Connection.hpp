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
	
private:
	void dataReceived(const char *&p, const char *read_buff_end);
	void sendPosition(unsigned int ra_int, int dec_int, int status) {Q_UNUSED(ra_int); Q_UNUSED(dec_int); Q_UNUSED(status);}
	void resetCommunication();
	
private:
	list<Ultima2000Command*> command_list;
};

#endif //_ULTIMA_2000_CONNECTION_HPP_
