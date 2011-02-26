/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2009-2011 Bogdan Marinov (this file,
 * reusing code written by Johannes Gajdosik in 2006)
 * 
 * Johannes Gajdosik wrote in 2006 the original telescope control feature
 * as a core module of Stellarium. In 2009 it was significantly extended with
 * GUI features and later split as an external plug-in module by Bogdan Marinov.
 * 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
#include "TelescopeClientDirectUltima2000.hpp"

#include "Ultima2000Connection.hpp"
#include "Ultima2000Command.hpp"
#include "LogFile.hpp"

#include <QRegExp>
#include <QStringList>

TelescopeClientDirectUltima2000::TelescopeClientDirectUltima2000(const QString &name, const QString &parameters, Equinox eq) :
		TelescopeClient(name),
		equinox(eq)
{
	interpolatedPosition.reset();
	
	//Extract parameters
	//Format: "serial_port_name:time_delay"
	QRegExp paramRx("^([^:]*):(\\d+)$");
	QString serialDeviceName;
	if (paramRx.exactMatch(parameters))
	{
		// This QRegExp only matches valid integers
		serialDeviceName = paramRx.capturedTexts().at(1).trimmed();
		time_delay       = paramRx.capturedTexts().at(2).toInt();
	}
	else
	{
		qWarning() << "ERROR creating TelescopeClientDirectUltima2000: invalid parameters.";
		return;
	}
	
	qDebug() << "TelescopeClientDirectUltima2000 paramaters: port, time_delay:" << serialDeviceName << time_delay;
	
	//Validation: Time delay
	if (time_delay <= 0 || time_delay > 10000000)
	{
		qWarning() << "ERROR creating TelescopeClientDirectUltima2000: time_delay not valid (should be less than 10000000)";
		return;
	}
	
	//end_of_timeout = -0x8000000000000000LL;
	
	#ifdef Q_OS_WIN32
	if(serialDeviceName.right(serialDeviceName.size() - 3).toInt() > 9)
		serialDeviceName = "\\\\.\\" + serialDeviceName;//"\\.\COMxx", not sure if it will work
	else
		serialDeviceName = serialDeviceName;
	#endif //Q_OS_WIN32
	
	//Try to establish a connection to the telescope
	ultima2000 = new Ultima2000Connection(*this, qPrintable(serialDeviceName));
	if (ultima2000->isClosed())
	{
		qWarning() << "ERROR creating TelescopeClientDirectUltima2000: cannot open serial device" << serialDeviceName;
		return;
	}
	
	//This connection will be deleted in the destructor of Server
	addConnection(ultima2000);
	
	queue_get_position = true;
	next_pos_time = -0x8000000000000000LL;
}

//! queues a GOTO command
void TelescopeClientDirectUltima2000::telescopeGoto(const Vec3d &j2000Pos)
{
	if (!isConnected())
		return;

	Vec3d position = j2000Pos;
	if (equinox == EquinoxJNow)
	{
		const StelNavigator* navigator = StelApp::getInstance().getCore()->getNavigator();
		position = navigator->j2000ToEquinoxEqu(j2000Pos);
	}

	//if (writeBufferEnd - writeBuffer + 20 < (int)sizeof(writeBuffer))
	//TODO: See the else clause, think how to do the same thing
	{
		const double ra_signed = atan2(position[1], position[0]);
		//Workaround for the discrepancy in precision between Windows/Linux/PPC Macs and Intel Macs:
		const double ra = (ra_signed >= 0) ? ra_signed : (ra_signed + 2.0 * M_PI);
		const double dec = atan2(position[2], sqrt(position[0]*position[0]+position[1]*position[1]));
		unsigned int ra_int = (unsigned int)floor(0.5 + ra*(((unsigned int)0x80000000)/M_PI));
		int dec_int = (int)floor(0.5 + dec*(((unsigned int)0x80000000)/M_PI));

		gotoReceived(ra_int, dec_int);
	}
	/*
		else
		{
			qDebug() << "TelescopeTCP(" << name << ")::telescopeGoto: "<< "communication is too slow, I will ignore this command";
		}
	*/
}

void TelescopeClientDirectUltima2000::gotoReceived(unsigned int ra_int, int dec_int)
{
	ultima2000->sendGoto(ra_int, dec_int);
}

//! estimates where the telescope is by interpolation in the stored
//! telescope positions:
Vec3d TelescopeClientDirectUltima2000::getJ2000EquatorialPos(const StelNavigator*) const
{
	const qint64 now = getNow() - time_delay;
	return interpolatedPosition.get(now);
}

bool TelescopeClientDirectUltima2000::prepareCommunication()
{
	//TODO: Nothing to prepare?
	return true;
}

void TelescopeClientDirectUltima2000::performCommunication()
{
	step(10000);
}

void TelescopeClientDirectUltima2000::communicationResetReceived(void)
{
	queue_get_position = true;
	next_pos_time = -0x8000000000000000LL;
	
#ifndef QT_NO_DEBUG
	*log_file << Now() << "TelescopeClientDirectUltima2000::communicationResetReceived" << endl;
#endif
}

//! Called by Ultima2000CommandGetRaDec::readAnswerFromBuffer().
void TelescopeClientDirectUltima2000::raDecReceived(unsigned int ra_int, unsigned int dec_int)
{
#ifndef QT_NO_DEBUG
	*log_file << Now() << "TelescopeClientDirectUltima2000::raDecReceived: " << ra_int << ", " << dec_int << endl;
#endif

	const int serial_status = 0;
	sendPosition(ra_int, dec_int, serial_status);
	queue_get_position = true;
}

void TelescopeClientDirectUltima2000::step(long long int timeout_micros)
{
	long long int now = GetNow();
	if (queue_get_position && now >= next_pos_time)
	{
		ultima2000->sendCommand(new Ultima2000CommandGetRaDec(*this));
		queue_get_position = false;
		next_pos_time = now + 500000;
	}
	Server::step(timeout_micros);
}

bool TelescopeClientDirectUltima2000::isConnected(void) const
{
	return (!ultima2000->isClosed());//TODO
}

bool TelescopeClientDirectUltima2000::isInitialized(void) const
{
	return (!ultima2000->isClosed());
}

//Merged from Connection::sendPosition() and TelescopeTCP::performReading()
void TelescopeClientDirectUltima2000::sendPosition(unsigned int ra_int, int dec_int, int status)
{
	//Server time is "now", because this class is the server
	const qint64 server_micros = (qint64) GetNow();
	const double ra  =  ra_int * (M_PI/(unsigned int)0x80000000);
	const double dec = dec_int * (M_PI/(unsigned int)0x80000000);
	const double cdec = cos(dec);
	Vec3d position(cos(ra)*cdec, sin(ra)*cdec, sin(dec));
	Vec3d j2000Position = position;
	if (equinox == EquinoxJNow)
	{
		const StelNavigator* navigator = StelApp::getInstance().getCore()->getNavigator();
		j2000Position = navigator->equinoxEquToJ2000(position);
	}
	interpolatedPosition.add(j2000Position, getNow(), server_micros, status);
}
