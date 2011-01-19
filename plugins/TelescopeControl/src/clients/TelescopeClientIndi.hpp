/*
 * Stellarium Telescope Control plug-in
 * Copyright (C) 2010  Bogdan Marinov
 * Copyright (C) 2011  Timothy Reaves
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
	static TelescopeClientIndi* telescopeClient(const QString& name, const QString& driverName, Equinox eq);
	static TelescopeClientIndi* telescopeClient(const QString& name, const QString& host, int port, Equinox eq);
	virtual ~TelescopeClientIndi();
	virtual bool isConnected() const = 0;
	virtual bool isInitialized() const = 0;

	//! Estimates a current position from the stored previous positions.
	//! InterpolatedPosition is used to make the apparent movement of the
	//! telescope reticle smoother.
	Vec3d getJ2000EquatorialPos(const StelNavigator *nav) const;
	void telescopeGoto(const Vec3d &j2000Pos);

signals:

protected slots:
	void handlePropertyDefinition(const QString& device, Property *property);
	void handlePropertyUpdate(const QString& device, Property* property);

protected:
	TelescopeClientIndi(const QString& name, Equinox eq);
	virtual bool prepareCommunication() = 0;
	virtual void performCommunication() = 0;
	InterpolatedPosition interpolatedPosition;
	virtual bool hasKnownPosition(void) const
	{
		return interpolatedPosition.isKnown();
	}

	Equinox equinox;

	IndiClient indiClient;

	bool isDefinedConnection;
	bool isConnectionConnected;
	bool isDefinedJ2000CoordinateRequest;
	bool isDefinedJNowCoordinateRequest;

	bool hasQueuedGoto;
	Vec3d queuedGotoJ2000Pos;
	
	virtual void initialize();
};

#endif //_TELESCOPE_CLIENT_INDI_HPP_
