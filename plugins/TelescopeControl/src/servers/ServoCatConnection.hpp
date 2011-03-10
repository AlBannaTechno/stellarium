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

#ifndef _SERVOCAT_CONNECTION_HPP_
#define _SERVOCAT_CONNECTION_HPP_

#include "SerialPort.hpp"

#include <list>
using namespace std;

class ServoCatCommand;

//! Serial port connection to a StellarCAT ServoCAT.
class ServoCatConnection : public SerialPort
{
public:
	ServoCatConnection(Server &server, const char *serial_device);
	~ServoCatConnection() { resetCommunication(); }
	void sendGoto(unsigned int ra_int, int dec_int);
	void sendCommand(ServoCatCommand * command);
	
private:
	void dataReceived(const char *&p, const char *read_buff_end);
	void sendPosition(unsigned int ra_int, int dec_int, int status) {Q_UNUSED(ra_int); Q_UNUSED(dec_int); Q_UNUSED(status);}
	void resetCommunication();
	
private:
	list<ServoCatCommand*> command_list;
};

#endif //_SERVOCAT_CONNECTION_HPP_
