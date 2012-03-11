/*
 * Stellarium Telescope Control plug-in
 * Copyright (C) 2010-2012  Bogdan Marinov
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
#include "IndiProperty.hpp"

#include <QObject>
#include <QString>

//! Base class for telescope clients that use the INDI protocol.
//! The INDI protocol is an XML-like, platform-independent interface for
//! communication between (astronomical) device drivers and clients.
//! This implementation allows communication via TCP/IP and input/output streams
//! on all platforms, though for now there are no INDI drivers for Windows.
//! An actual connection is implemented in TelescopeClientIndiLocal (runs an
//! INDI driver in a separate process) and TelescopeClientIndiTcp (connects over
//! a network connection to an INDI server).
//! An actual TelescopeClient that can read the commands and display a reticle
//! is implemented in TelescopeClientIndiPointer.
//! This class' implementation of the standard TelescopeClient methods is based
//! on a limited subset of the INDI "standard property" names (see
//! http://www.indilib.org/index.php?title=Standard_Properties):
//!  - CONNECTION
//!  - EQUATORIAL_COORD
//!  - EQUATORIAL_EOD_COORD
//! \todo Support custom property names.
//! \todo Get rid of the auto-connection.
//! \todo Major overhaul of the main logic and handling of INDI client management.
class TelescopeClientIndi : public TelescopeClient
{
	Q_OBJECT
public:
	//! Creates a TelescopeClientIndiLocal.
	static TelescopeClientIndi* telescopeClient(const QString& name, const QString& driverName, Equinox eq);
	//! Creates a TelescopeClientIndiTcp.
	static TelescopeClientIndi* telescopeClient(const QString& name, const QString& host, int port, Equinox eq);
	//! Creates a TelescopeClientIndiPointer.
	static TelescopeClientIndi* telescopeClient(const QString& name,
	                                            const QString& deviceId,
	                                            IndiClient* client,
	                                            Equinox eq);
	virtual ~TelescopeClientIndi();
	virtual bool isConnected() const = 0;
	virtual bool isInitialized() const = 0;

	//! Estimates a current position from the stored previous positions.
	//! InterpolatedPosition is used to make the apparent movement of the
	//! telescope reticle smoother.
	Vec3d getJ2000EquatorialPos(const StelCore* core) const;
	void telescopeGoto(const Vec3d &j2000Pos);

	IndiClient* getIndiClient() const;

signals:

protected slots:
	//! If the device matches the wanted one, connect to it and wait for properties.
	//! \todo Deal with this on management level? A client is useless without a device.
	void handleDeviceDefinition(const QString& client, const DeviceP& newDevice);
	//! If the property is a useful standard property, connect to it.
	//! \todo Save a pointer to the property.
	//! \todo Connect each type of property to the appropriate handling function.
	void handlePropertyDefinition(const PropertyP& property);
	//! 
	void updatePositionFromProperty();
	//! 
	void handleConnectionEstablished();

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

	IndiClient* indiClient;
	//! The name identifying this device's properties.
	//! Properties belonging to other devices are ignored.
	QString deviceName;
	//! 
	DeviceP device;

	bool isDefinedConnection;
	bool isConnectionConnected;
	SwitchPropertyP connectionProp;
	
	//! Property holding the current position.
	//! J2000 properties are preferred, if not JNow.
	//! \todo Handle altaz.
	NumberPropertyP posProp;
	NumberPropertyP requestedPosProp;
	bool isDefinedJ2000CoordinateRequest;
	bool isDefinedJNowCoordinateRequest;

	bool hasQueuedGoto;
	Vec3d queuedGotoJ2000Pos;
	
	virtual void initialize();
};

#endif //_TELESCOPE_CLIENT_INDI_HPP_
