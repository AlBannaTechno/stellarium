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

#include "Ultima2000Connection.hpp"
#include "Ultima2000Command.hpp"
#include "TelescopeClientDirectUltima2000.hpp"
#include "LogFile.hpp"

#include <iostream>
using namespace std;

Ultima2000Connection::Ultima2000Connection(Server &server, const char *serial_device) : SerialPort(server, serial_device)
{
}

void Ultima2000Connection::resetCommunication()
{
	while (!command_list.empty())
	{
		delete command_list.front();
		command_list.pop_front();
	}
	
	read_buff_end = read_buff;
	write_buff_end = write_buff;
#ifdef DEBUG4
	*log_file << Now() << "Ultima2000Connection::resetCommunication" << endl;
#endif
}

void Ultima2000Connection::sendGoto(unsigned int ra32, int dec32)
{
	//Conversion
	//This is a bit stupid...
	quint16 ra16 = (quint16)floor(ra32 / 65536.0);
	qint16 dec16 = (qint16)floor(dec32 / 65536.0);

	sendCommand(new Ultima2000CommandGotoPosition(server, ra16, dec16));
}

void Ultima2000Connection::dataReceived(const char *&p,const char *read_buff_end)
{
	if (isClosed())
	{
		*log_file << Now() << "Ultima2000Connection::dataReceived: strange: fd is closed" << endl;
	}
	else if (command_list.empty())
	{
		#ifdef DEBUG4
		*log_file << Now() << "Ultima2000Connection::dataReceived: "
		                      "error: command_list is empty" << endl;
		#endif
		resetCommunication();
		static_cast<TelescopeClientDirectUltima2000*>(&server)->communicationResetReceived();
	}
	else if (command_list.front()->needsNoAnswer())
	{
		*log_file << Now() << "Ultima2000Connection::dataReceived: "
		                      "strange: command(" << *command_list.front()
		                   << ") needs no answer" << endl;
	}
	else
	{
		while(true)
		{
			const int rc=command_list.front()->readAnswerFromBuffer(p, read_buff_end);
			//*log_file << Now() << "Ultima2000Connection::dataReceived: "
			//                   << *command_list.front() << "->readAnswerFromBuffer returned "
			//                   << rc << endl;
			if (rc <= 0)
			{
				if (rc < 0)
				{
					resetCommunication();
					static_cast<TelescopeClientDirectUltima2000*>(&server)->communicationResetReceived();
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

void Ultima2000Connection::sendCommand(Ultima2000Command *command)
{
	if (command)
	{
		#ifdef DEBUG4
		*log_file << Now() << "Ultima2000Connection::sendCommand(" << *command
			  << ")" << endl;
		#endif
			command_list.push_back(command);
		while (!command_list.front()->hasBeenWrittenToBuffer())
		{
			if (command_list.front()->writeCommandToBuffer(
				                          write_buff_end,
				                          write_buff+sizeof(write_buff)))
			{
				//*log_file << Now() << "Ultima2000Connection::sendCommand: "
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
				//*log_file << Now() << "Ultima2000Connection::sendCommand: "
				//                   << (*command_list.front())
				//                   << "::writeCommandToBuffer failed" << endl;
				break;
			}
		}
		//*log_file << Now() << "Ultima2000Connection::sendCommand(" << *command << ") end"
		//                   << endl;
	}
}

