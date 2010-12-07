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

#include "ServoCatConnection.hpp"
#include "ServoCatCommand.hpp"
#include "TelescopeClientDirectServoCat.hpp"
#include "LogFile.hpp"

#include <iostream>
using namespace std;

ServoCatConnection::ServoCatConnection(Server &server, const char *serial_device) :
		SerialPort(server, serial_device)
{
}

void ServoCatConnection::resetCommunication(void)
{
	while (!command_list.empty())
	{
		delete command_list.front();
		command_list.pop_front();
	}
	
	read_buff_end = read_buff;
	write_buff_end = write_buff;
#ifdef DEBUG4
	*log_file << Now() << "ServoCatConnection::resetCommunication" << endl;
#endif
}

void ServoCatConnection::sendGoto(unsigned int ra_int, int dec_int)
{
	//De-conversion from the strange way of encoding
	double dec = 0.5 + (dec_int * 360.0)/4294967296.0;
	if (dec < -90.0)
	{
		dec = -180.0 - dec;
		ra_int += 0x80000000;
	}
	else if (dec > 90.0)
	{
		dec = 180.0 - dec;
		ra_int += 0x80000000;
	}
	double ra = 0.5 + (ra_int * 24.0) / 4294967296.0;
	if (ra >= 24.0)
		ra -= 24.0;

	sendCommand(new ServoCatCommandGotoPosition(server, ra, dec));
}

void ServoCatConnection::dataReceived(const char *&p,const char *read_buff_end)
{
	if (isClosed())
	{
		*log_file << Now() << "ServoCatConnection::dataReceived: strange: fd is closed" << endl;
	}
	else if (command_list.empty())
	{
		#ifdef DEBUG4
		*log_file << Now() << "ServoCatConnection::dataReceived: "
		                      "error: command_list is empty" << endl;
		#endif
		resetCommunication();
		static_cast<TelescopeClientDirectServoCat*>(&server)->communicationResetReceived();
	}
	else if (command_list.front()->needsNoAnswer())
	{
		*log_file << Now() << "ServoCatConnection::dataReceived: "
		                      "strange: command(" << *command_list.front()
		                   << ") needs no answer" << endl;
	}
	else
	{
		while(true)
		{
			const int rc=command_list.front()->readAnswerFromBuffer(p, read_buff_end);
			//*log_file << Now() << "ServoCatConnection::dataReceived: "
			//                   << *command_list.front() << "->readAnswerFromBuffer returned "
			//                   << rc << endl;
			if (rc <= 0)
			{
				if (rc < 0)
				{
					resetCommunication();
					static_cast<TelescopeClientDirectServoCat*>(&server)->communicationResetReceived();
				}
				break;
			}
			delete command_list.front();
			command_list.pop_front();
			if (command_list.empty())
				break;
			if (!command_list.front()->writeCommandToBuffer(
			                                   write_buff_end,
			                                   write_buff+sizeof(write_buff)))
				break;
		}
	}
}

void ServoCatConnection::sendCommand(ServoCatCommand *command)
{
	if (command)
	{
		#ifdef DEBUG4
		*log_file << Now() << "ServoCatConnection::sendCommand(" << *command
			  << ")" << endl;
		#endif
			command_list.push_back(command);
		while (!command_list.front()->hasBeenWrittenToBuffer())
		{
			if (command_list.front()->writeCommandToBuffer(
				                          write_buff_end,
				                          write_buff+sizeof(write_buff)))
			{
				//*log_file << Now() << "ServoCatConnection::sendCommand: "
				//                   << (*command_list.front())
				//                   << "::writeCommandToBuffer ok" << endl;
				if (command_list.front()->needsNoAnswer())
				{
					delete command_list.front();
					command_list.pop_front();
					if (command_list.empty())
						break;
				}
				else
				{
					break;
				}
			}
			else
			{
				//*log_file << Now() << "ServoCatConnection::sendCommand: "
				//                   << (*command_list.front())
				//                   << "::writeCommandToBuffer failed" << endl;
				break;
			}
		}
		//*log_file << Now() << "ServoCatConnection::sendCommand(" << *command << ") end"
		//                   << endl;
	}
}

