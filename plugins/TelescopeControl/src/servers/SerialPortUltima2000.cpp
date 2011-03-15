/*
Stellarium telescope control
Copyright (C) 2006 Johannes Gajdosik
Copyright (C) 2011 Bogdan Marinov (Ultima2000 modifications)

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

#include "SerialPortUltima2000.hpp"
#include "LogFile.hpp"

#ifndef Q_OS_WIN32
#include <unistd.h>
#endif

#include <string.h> // memset

using namespace std;

SerialPortUltima2000::SerialPortUltima2000(Server &server,
                                           const char *serial_device)
	: Connection(server, INVALID_SOCKET)
{
#ifdef Q_OS_WIN32
	handle = CreateFile(serial_device,
                       	GENERIC_READ|GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if (handle == INVALID_HANDLE_VALUE)
	{
		*log_file << Now() << "SerialPortUltima2000::SerialPortUltima2000(" 
		                   << serial_device << "): CreateFile() failed: "
		                   << GetLastError() << endl;
	}
	else
	{
		COMMTIMEOUTS timeouts;
		timeouts.ReadIntervalTimeout = MAXDWORD; 
		timeouts.ReadTotalTimeoutMultiplier = 0;
		timeouts.ReadTotalTimeoutConstant = 0;
		timeouts.WriteTotalTimeoutMultiplier = 0;
		timeouts.WriteTotalTimeoutConstant = 0;
		if (!SetCommTimeouts(handle, &timeouts))
		{
			*log_file << Now() << "SerialPortUltima2000::SerialPortUltima2000("
			          << serial_device << "): SetCommTimeouts() failed: "
			          << GetLastError() << endl;
		}
		else
		{
			if (!GetCommState(handle, &dcb_original))
			{
				*log_file << Now()
				          << "SerialPortUltima2000::SerialPortUltima2000("
				          << serial_device << "): GetCommState() failed: "
				          << GetLastError() << endl;
			}
			else
			{
				DCB dcb;
				memset(&dcb, 0, sizeof(dcb));
				dcb.DCBlength = sizeof(dcb);
				if (!BuildCommDCB("9600,n,8,1", &dcb))
				{
					*log_file << Now()
					          << "SerialPortUltima2000::SerialPortUltima2000("
					          << serial_device << "): BuildCommDCB() failed: "
					          << GetLastError() << endl;
				}
				else
				{
					//Turn off all possible flow of control flags
					dcb.fOutxCtsFlow = FALSE;
					dcb.fOutxDsrFlow = FALSE;
					dcb.fDsrSensitivity = FALSE;
					dcb.fDtrControl = DTR_CONTROL_DISABLE;
					dcb.fRtsControl = RTS_CONTROL_DISABLE;
					*log_file << "SerialPortUltima2000: Default EOF character: "
					          << (int)dcb.EofChar << endl;

					if (!SetCommState(handle,&dcb))
					{
						*log_file << Now()
						          << "SerialPort::SerialPortUltima2000("
						          << serial_device
						          << "): SetCommState() failed: "
						          << GetLastError() << endl;
					}
					else
					{
						// success
						return;
					}
				}
			}
		}
		CloseHandle(handle);
		handle = INVALID_HANDLE_VALUE;
	}
#else
	fd = open(serial_device, O_RDWR|O_NOCTTY);
	if (fd < 0)
	{
		*log_file << Now() << "SerialPortUltima2000::SerialPortUltima2000("
		          << serial_device << "): open() failed: "
		          << strerror(errno) << endl;
	}
	else
	{
		if (SETNONBLOCK(fd) < 0)
		{
			*log_file << Now() << "SerialPortUltima2000::SerialPortUltima2000("
			          << serial_device << "): fcntl(O_NONBLOCK) failed: "
			          << STRERROR(ERRNO) << endl;
		}
		else
		{
			if (tcgetattr(fd,&termios_original) < 0)
			{
				*log_file << Now()
				          << "SerialPortUltima2000::SerialPortUltima2000("
				          << serial_device << "): tcgetattr failed: "
				          << strerror(errno) << endl;
			}
			else
			{
				struct termios termios_new;
				memset(&termios_new, 0, sizeof(termios_new));
				termios_new.c_cflag = CS8    |  // 8 data bits
				                      // no parity because PARENB is not set
				                      CLOCAL |  // Ignore modem control lines
				                      CREAD;    // Enable receiver
				cfsetospeed(&termios_new, B9600);
				termios_new.c_lflag = 0;
				termios_new.c_cc[VTIME] = 0;
				termios_new.c_cc[VMIN] = 1;
				if (tcsetattr(fd,TCSAFLUSH,&termios_new) < 0)
				{
					*log_file << Now()
					          << "SerialPortUltima2000::SerialPortUltima2000("
					          << serial_device << "): tcsetattr failed: "
					          << strerror(errno) << endl;
				}
				else
				{
					// success
					return;
				}
			}
		}
		close(fd);
		fd = -1;
	}
#endif //Q_OS_WIN32
}

SerialPortUltima2000::~SerialPortUltima2000()
{
#ifdef Q_OS_WIN32
	if (handle != INVALID_HANDLE_VALUE)
	{
		// restore original settings
		SetCommState(handle, &dcb_original);
		CloseHandle(handle);
	}
#else
	if (fd >= 0)
	{
		// restore original settings
		tcsetattr(fd, TCSANOW, &termios_original);
		close(fd);
	}
#endif
}


#ifdef Q_OS_WIN32

int SerialPortUltima2000::readNonblocking(char *buf, int count)
{
	DWORD rval;
	if (ReadFile(handle, buf, count, &rval, 0))
		return (int)rval;
	if (GetLastError() == ERROR_IO_PENDING)
		return 0;
	return -1;
}

int SerialPortUltima2000::writeNonblocking(const char *buf, int count)
{
	DWORD rval;
	if (WriteFile(handle, buf, count, &rval, 0))
		return (int)rval;
	if (GetLastError() == ERROR_IO_PENDING)
		return 0;
	return -1;
}

#endif

void SerialPortUltima2000::prepareSelectFds(fd_set &read_fds,
                                  fd_set &write_fds,
                                  int &fd_max)
{
#ifdef Q_OS_WIN32
	// handle all IO here
	if (write_buff_end > write_buff)
		performWriting();
	performReading();
#else
	Connection::prepareSelectFds(read_fds, write_fds, fd_max);
#endif //Q_OS_WIN32
}
