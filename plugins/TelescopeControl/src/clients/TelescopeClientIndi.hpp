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

class IndiServices;

//! \ingroup telescope-markers
//! Base class for telescope clients that use the INDI protocol.
//! The INDI protocol is an XML-like, platform-independent interface for
//! communication between (astronomical) device drivers and clients.
//! 
//! This class' implementation of the standard TelescopeClient methods is based
//! on a limited subset of the INDI "standard property" names (see
//! http://www.indilib.org/index.php?title=Standard_Properties):
//!  - CONNECTION
//!  - EQUATORIAL_COORD
//!  - EQUATORIAL_EOD_COORD
//! 
//! TelescopeClientIndi objects reuse IndiClient objects. Ideally, you should
//! have only one instance of IndiClient for the local server and one per each
//! connection to a remote server.
//! \todo Support custom property names.
//! \todo Get rid of the auto-connection.
//! \todo Major overhaul of the main logic and handling of INDI client management.
//! \todo Expand class documentation.
class TelescopeClientIndi : public TelescopeClient
{
	Q_OBJECT
public:
	//! Constructor.
	//! \param name standard Stellarium object displayable name/identifier.
	//! \param deviceName the name/identifier of the device this object is
	//! supposed to represent.
	//! \param client if 0, it will wait for attachClient().
	TelescopeClientIndi(const QString& name,
	                    const QString& deviceName,
	                    IndiClient* client = 0);
	
	//! Creates a TelescopeClientIndiPointer.
	//! \deprecated
	static TelescopeClientIndi* telescopeClient(const QString& name,
	                                            const QString& deviceId,
	                                            IndiClient* client, Equinox eq);
	virtual ~TelescopeClientIndi();
	
	//! \returns true if the object has been initialized correctly.
	virtual bool isInitialized() const;
	//! \todo Decide how to define "connected".
	virtual bool isConnected() const;

	//! Estimates a current position from the stored previous positions.
	//! InterpolatedPosition is used to make the apparent movement of the
	//! telescope reticle smoother.
	Vec3d getJ2000EquatorialPos(const StelCore* core) const;
	void telescopeGoto(const Vec3d &j2000Pos);

	IndiClient* getIndiClient() const;
	QString getIndiDeviceId() const;

public slots:
	//! \todo Use this for all kinds of INDI telescope clients?
	void attachClient(IndiClient* client);
	
signals:
	//! Emitted if a property defining coordinates has been defined.
	//! The parameter is because I didn't want to bother with a signal mapper.:)
	//! \todo Better name?
	//! \todo Remove the parameter and use a signal mapper in TelescopeControl
	//! to separate the internal client name from the client ID in the hash?
	void coordinatesDefined();
	void coordinatesUndefined();

protected slots:
	//! If the device matches the wanted one, connect to it and wait for properties.
	//! \todo Deal with this on management level? A client is useless without a device.
	void handleDeviceDefinition(const QString& client, const DeviceP& newDevice);
	//! 
	void handleDeviceRemoval(const QString& client, const QString& devName);
	//! If the property is a useful standard property, connect to it.
	//! \todo Save a pointer to the property.
	//! \todo Connect each type of property to the appropriate handling function.
	void handlePropertyDefinition(const PropertyP& property);
	//! 
	void handlePropertyRemoval(const QString& propertyName);
	//! 
	void updatePositionFromProperty();
	//! 
	void handleConnectionEstablished();
	//!
	void handleClientFailure();

protected:
	TelescopeClientIndi(const QString& name, Equinox eq);
	virtual bool prepareCommunication();
	virtual void performCommunication();
	InterpolatedPosition interpolatedPosition;
	virtual bool hasKnownPosition() const;

	IndiClient* indiClient;
	bool waitingForClient;
	
	//! The name identifying this device's properties.
	//! Properties belonging to other devices are ignored.
	QString deviceName;
	//! 
	DeviceP device;

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
