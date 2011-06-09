/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2006 Johannes Gajdosik
 * Copyright (C) 2009-2010 Bogdan Marinov
 * 
 * This module was originally written by Johannes Gajdosik in 2006
 * as a core module of Stellarium. In 2009 it was significantly extended with
 * GUI features and later split as an external plug-in module by Bogdan Marinov.
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
#ifndef _TELESCOPE_CLIENT_TCP_HPP_
#define _TELESCOPE_CLIENT_TCP_HPP_

#include "TelescopeClient.hpp"

#include <QHostAddress>
#include <QHostInfo>
#include <QObject>
#include <QTcpSocket>

#include "InterpolatedPosition.hpp"

//! This TelescopeClient class can controll a telescope by communicating
//! to a server process ("telescope server") via 
//! the "Stellarium telescope control protocol" over TCP/IP.
//! The "Stellarium telescope control protocol" is specified in a seperate
//! document along with the telescope server software.
class TelescopeClientTcp : public TelescopeClient
{
	Q_OBJECT
public:
	TelescopeClientTcp(const QString &name, const QString &params, Equinox eq = EquinoxJ2000);
	~TelescopeClientTcp(void)
	{
		hangup();
	}
	bool isConnected(void) const
	{
		//return (tcpSocket->isValid() && !wait_for_connection_establishment);
		return (tcpSocket->state() == QAbstractSocket::ConnectedState);
	}
	bool isInitialized(void) const
	{
		return (!address.isNull());
	}
	
private:
	Vec3d getJ2000EquatorialPos(const StelCore* core=0) const;
	bool prepareCommunication();
	void performCommunication();
	void telescopeGoto(const Vec3d &j2000Pos);
	void performReading(void);
	void performWriting(void);
	
private:
	void hangup(void);
	QHostAddress address;
	unsigned int port;
	QTcpSocket * tcpSocket;
	bool wait_for_connection_establishment;
	qint64 end_of_timeout;
	char readBuffer[120];
	char *readBufferEnd;
	char writeBuffer[120];
	char *writeBufferEnd;
	int time_delay;

	InterpolatedPosition interpolatedPosition;
	virtual bool hasKnownPosition(void) const
	{
		return interpolatedPosition.isKnown();
	}

	Equinox equinox;
	
private slots:
	void socketFailed(QAbstractSocket::SocketError socketError);
};

#endif //_TELESCOPE_CLIENT_TCP_HPP_
