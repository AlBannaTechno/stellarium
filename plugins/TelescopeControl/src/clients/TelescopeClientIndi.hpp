/*
 * Stellarium Telescope Control plug-in
 * Copyright (C) 2010  Bogdan Marinov (this file)
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TELESCOPE_CLIENT_INDI_HPP_
#define _TELESCOPE_CLIENT_INDI_HPP_

#include "TelescopeClient.hpp"
#include "InterpolatedPosition.hpp"
#include "IndiClient.hpp"

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTcpSocket>

//! Telescope client that uses the INDI protocol.
//! The INDI protocol is an XML-like, platform-independent interface for
//! communication between (astronomical) device drivers and clients.
//! This implementation allows communication via TCP/IP and input/output streams
//! on all platforms, though for now there are no INDI drivers for Windows.
//! This class includes a simplified client that covers a limited subset of
//! the INDI "standard" properties (see
//! http://www.indilib.org/index.php?title=Standard_Properties):
//!  - CONNECTION
//!  - EQUATORIAL_COORD
//!  - EQUATORIAL_EOD_COORD
class TelescopeClientIndi : public TelescopeClient
{
	Q_OBJECT
public:
	TelescopeClientIndi(const QString& name, const QString& params, Equinox eq);
	virtual ~TelescopeClientIndi(void);
	bool isConnected(void) const;
	bool isInitialized(void) const;

	//! Estimates a current position from the stored previous positions.
	//! InterpolatedPosition is used to make the apparent movement of the
	//! telescope reticle smoother.
	Vec3d getJ2000EquatorialPos(const StelNavigator *nav) const;
	bool prepareCommunication();
	void performCommunication();
	void telescopeGoto(const Vec3d &j2000Pos);

signals:

private slots:
	void handleDriverError(QProcess::ProcessError error);
	void handleConnectionError(QAbstractSocket::SocketError error);

	void handlePropertyUpdate(QString device, Property* property);

private:
	InterpolatedPosition interpolatedPosition;
	virtual bool hasKnownPosition(void) const
	{
		return interpolatedPosition.isKnown();
	}

	Equinox equinox;

	//! Counter for taking the telescope's position every
	//! POSITION_REFRESH_INTERVAL microseconds.
	qint64 timeToGetPosition;
	//! Interval between attempts to read position info, in microseconds.
	//! Default is half a second.
	static const qint64 POSITION_REFRESH_INTERVAL = 500000;

	bool isRemoteConnection;
	IndiClient indiClient;
	QTcpSocket* tcpSocket;
	QProcess* driverProcess;
};

#endif //_TELESCOPE_CLIENT_INDI_HPP_
