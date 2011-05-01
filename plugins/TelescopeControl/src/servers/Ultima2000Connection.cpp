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
#include <cmath>
using namespace std;

Ultima2000Connection::Ultima2000Connection(Server &server,
                                           const char *serial_device)
	: SerialPortUltima2000(server, serial_device)
{
	time_between_commands = 0;
	next_send_time = GetNow();
	read_timeout_endtime = 0x7FFFFFFFFFFFFFFFLL;
	goto_commands_queued = 0;
}

void Ultima2000Connection::resetCommunication()
{
	while (!commandQueue.empty())
	{
		delete commandQueue.front();
		commandQueue.pop_front();
	}
	
	read_buff_end = read_buff;
	write_buff_end = write_buff;
#ifdef DEBUG4
	*log_file << Now()
	          << "Ultima2000Connection::resetCommunication"
	          << endl;
#endif
	// wait 10 seconds before sending the next command in order to read
	// and ignore data coming from the telescope:
	next_send_time = GetNow() + 10000000;
	read_timeout_endtime = 0x7FFFFFFFFFFFFFFFLL;
	goto_commands_queued = 0;
	static_cast<TelescopeClientDirectUltima2000&>(server).communicationResetReceived();
}

void Ultima2000Connection::sendGoto(unsigned int ra32, int dec32)
{
	if (goto_commands_queued <= 1)
	{
		//Conversion
		//This is a bit stupid...
		quint16 ra16 = (quint16)floor(ra32 / 65536.0);
		qint16 dec16 = (qint16)floor(dec32 / 65536.0);

		sendCommand(new Ultima2000CommandGotoPosition(server, ra16, dec16));
		goto_commands_queued++;
	}
	else
	{
#ifdef DEBUG4
		*log_file << Now()
		          << "Ultima2000Connection::sendGoto: "
		          << "Too much GOTO commands in queue; "
		          << "The last one has been ignored." << endl;
#endif
	}
}

bool Ultima2000Connection::writeFrontCommandToBuffer()
{
	if(commandQueue.empty())
	{
		return false;
	}
	
	const long long int now = GetNow();
	if (now < next_send_time) 
	{
#ifdef DEBUG4
		/*
		*log_file << Now()
		          << "Ultima2000Connection::writeFrontCommandToBuffer("
		          << (*command_list.front()) << "): delayed for "
		          << (next_send_time-now) << endl;
		*/
#endif
		return false;
	}
	
	const bool rval = commandQueue.front()->
	                  writeCommandToBuffer(write_buff_end,
	                                       write_buff + sizeof(write_buff));
	if (rval)
	{
		next_send_time = now;
		if (commandQueue.front()->needsNoAnswer())
		{
			next_send_time += time_between_commands;
			read_timeout_endtime = 0x7FFFFFFFFFFFFFFFLL;
		}
		else
		{
			if (commandQueue.front()->isCommandGoto())
			{
				// shorter timeout for AutoStar 494 slew:
				read_timeout_endtime = now + 3000000;
			}
			else
			{
				// extra long timeout for AutoStar 494:
				read_timeout_endtime = now + 5000000;
			}
		}
		#ifdef DEBUG4
		*log_file << Now()
		          << "Ultima2000Connection::writeFrontCommandToBuffer("
		          << (*commandQueue.front())
		          << "): queued"
		          << endl;
		#endif
	}
	
	return rval;
}

void Ultima2000Connection::dataReceived(const char *&p,const char *read_buff_end)
{
	if (isClosed())
	{
		*log_file << Now()
		          << "Ultima2000Connection::dataReceived: strange: fd is closed"
		          << endl;
	}
	else if (commandQueue.empty())
	{
		if (GetNow() < next_send_time)
		{
			// just ignore
			p = read_buff_end;
		}
		else
		{
#ifdef DEBUG4
			*log_file << Now() << "Ultima2000Connection::dataReceived: "
			             "error: command_list is empty" << endl;
#endif
			resetCommunication();
			static_cast<TelescopeClientDirectUltima2000*>(&server)->communicationResetReceived();
		}
	}
	else if (commandQueue.front()->needsNoAnswer())
	{
		*log_file << Now() << "Ultima2000Connection::dataReceived: "
		          << "strange: command("
		          << *commandQueue.front()
			      << ") needs no answer."
		          << endl;
		p = read_buff_end;
	}
	else
	{
		while(true)
		{
			if (!commandQueue.front()->hasBeenWrittenToBuffer())
			{
				*log_file << Now()
				          << "Ultima2000Connection::dataReceived: "
				             "strange: no answer expected"
				          << endl;
				p = read_buff_end;
				break;
			}
			const int rc = commandQueue.front()->
			               readAnswerFromBuffer(p, read_buff_end);
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
			if (commandQueue.front()->isCommandGoto())
			{
				goto_commands_queued--;
			}
			delete commandQueue.front();
			commandQueue.pop_front();
			read_timeout_endtime = 0x7FFFFFFFFFFFFFFFLL;
			if (!writeFrontCommandToBuffer())
				break;
		}
	}
}

void Ultima2000Connection::prepareSelectFds(fd_set& read_fds,
                                            fd_set& write_fds,
                                            int& fd_max)
{
	// if some telegram is delayed try to queue it now:
	flushCommandList();
	if (!commandQueue.empty() && GetNow() > read_timeout_endtime)
	{
		/*if (command_list.front()->shortAnswerReceived())
		{
			// the lazy telescope, propably AutoStar 494
			// has not sent the full answer
			#ifdef DEBUG4
			*log_file << Now() << "Ultima2000Connection::prepareSelectFds: "
			                      "dequeueing command("
			                   << *command_list.front()
			                   << ") because of timeout"
			                   << endl;
			#endif
			if (command_list.front()->isCommandGoto())
			{
				goto_commands_queued--;
			}
			delete command_list.front();
			command_list.pop_front();
			read_timeout_endtime = 0x7FFFFFFFFFFFFFFFLL;
		}
		else*/
		{
			resetCommunication();
		}
	}
	SerialPortUltima2000::prepareSelectFds(read_fds, write_fds, fd_max);
}

void Ultima2000Connection::flushCommandList()
{
	if (!commandQueue.empty())
	{
		while (!commandQueue.front()->hasBeenWrittenToBuffer())
		{
			if (writeFrontCommandToBuffer())
			{
				//*log_file << Now()
				//          << "Ultima2000Connection::flushCommandList: "
				//          << (*command_list.front())
				//          << "::writeFrontCommandToBuffer ok"
				//          << endl;
				if (commandQueue.front()->needsNoAnswer())
				{
					delete commandQueue.front();
					commandQueue.pop_front();
					read_timeout_endtime = 0x7FFFFFFFFFFFFFFFLL;
					if (commandQueue.empty())
						break;
				}
				else
				{
					break;
				}
			}
			else
			{
				//*log_file << Now() << "Ultima2000Connection::flushCommandList: "
				//                   << (*command_list.front())
				//                   << "::writeFrontCommandToBuffer failed/delayed" << endl;
				break;
			}
		}
	}
}

void Ultima2000Connection::sendCommand(Ultima2000Command *command)
{
	if (command)
	{
#ifdef DEBUG4
		*log_file << Now()
		          << "Ultima2000Connection::sendCommand("
		          << *command
		          << ")"
		          << endl;
#endif
		commandQueue.push_back(command);
		flushCommandList();
		/*
		*log_file << Now()
		          << "Ultima2000Connection::sendCommand("
		          << *command
		          << ") end"
		          << endl;
		*/
	}
}

