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

#include "IndiClient.hpp"
#include "IndiServices.hpp"
#include "TelescopeClientIndi.hpp"
#include "TelescopeClientIndiPointer.hpp"

#include <cmath>
#include "StelUtils.hpp"

TelescopeClientIndi::TelescopeClientIndi(const QString& newName,
                                         const QString& newDeviceName,
                                         IndiClient* client) :
    TelescopeClient(newName),
    indiClient(0),
    waitingForClient(false),
    deviceName(newDeviceName),
    isConnectionConnected(false),
    isDefinedJ2000CoordinateRequest(false),
    isDefinedJNowCoordinateRequest(false),
    hasQueuedGoto(false)
{
	if (newName.isEmpty() || newDeviceName.isEmpty())
		return;
	
	if (client)
	{
		attachClient(client);
	}
	else
	{
		// If no IndiClient instance has been passed to the constructor,
		// wait for one to be passed to attachClient().
		waitingForClient = true;
	}
}

TelescopeClientIndi::~TelescopeClientIndi()
{
	//
}

bool TelescopeClientIndi::isInitialized() const
{
	// There could have been more checks...
	if (waitingForClient || indiClient)
		return true;
	else
		return false;
}

bool TelescopeClientIndi::isConnected() const
{
	//TODO: Should this include a check for the CONNECTION property?
	if (indiClient)
		return true;
	else
		return false;
}

bool TelescopeClientIndi::prepareCommunication()
{
	if (!isInitialized())
		return false;

	//TODO: Request properties?
	//TODO: Try to connect

	return true;
}

void TelescopeClientIndi::performCommunication()
{
	//
}

bool TelescopeClientIndi::hasKnownPosition() const
{
	return interpolatedPosition.isKnown();
}

void TelescopeClientIndi::initialize()
{
//	isConnectionConnected = false;
//	isDefinedJ2000CoordinateRequest = false;
//	 = false;
//	 = false;
}

IndiClient* TelescopeClientIndi::getIndiClient() const
{
	return indiClient;
}

void TelescopeClientIndi::attachClient(IndiClient* client)
{
	if(indiClient)
		return;
	
	if (!client)
	{
		qDebug() << "Telescope Control: Error: Trying to attach an empty client"
		         << " to TelescopeClientIndi" << name;
		return;
	}
	
	waitingForClient = false;
	indiClient = client;
	DeviceP device = indiClient->getDevice(deviceName);
	if (device.isNull())
	{
		// Wait for the device to be defined
		connect(indiClient, SIGNAL(deviceDefined(QString,DeviceP)),
		        this, SLOT(handleDeviceDefinition(QString,DeviceP)));
	}
	else
	{
		handleDeviceDefinition(indiClient->getId(), device);
		connect(indiClient, SIGNAL(deviceRemoved(QString,QString)),
		        this, SLOT(handleDeviceRemoval(QString,QString)));
	}
	// TODO: Connect error handling slot?
	// TODO: Extract device name?
	connect(indiClient, SIGNAL(aboutToClose()),
	        this, SLOT(handleClientFailure()));
}

Vec3d TelescopeClientIndi::getJ2000EquatorialPos(const StelCore *core) const
{
	Q_UNUSED(core);
	//TODO: see what to do about time_delay
	const qint64 now = getNow() - 1500000;// - time_delay;
	return interpolatedPosition.get(now);
}

void TelescopeClientIndi::telescopeGoto(const Vec3d &j2000Coordinates)
{
	if (!isInitialized())
		return;

	if (!isConnected())
	{
		return;
	}
	
	if (requestedPosProp.isNull())
		return;

	//If it's not connected, attempt to connect
	if (connectionProp && !isConnectionConnected)
	{
		hasQueuedGoto = true;
		queuedGotoJ2000Pos = j2000Coordinates;
		QHash<QString,bool> newValues;
		newValues.insert(IndiClient::SP_CONNECT, true);
		newValues.insert(IndiClient::SP_DISCONNECT, false);
		connectionProp->send(newValues);
		return;
	}

	//TODO: Verify what kind of action (SLEW, SYNC) is going to be performed on a request

	Vec3d targetCoordinates;
	if (isDefinedJ2000CoordinateRequest)
	{
		targetCoordinates = j2000Coordinates;
	}
	else if (isDefinedJNowCoordinateRequest)
	{
		const StelCore* core = StelApp::getInstance().getCore();
		targetCoordinates = core->j2000ToEquinoxEqu(j2000Coordinates);
	}
	else
	{
		//If no standard properties for requesting a slew are defined,
		//there's no way to perform one.
		return;
	}
	//Get a coordinate pair from the vector
	double raRadians;
	double decRadians;
	StelUtils::rectToSphe(&raRadians, &decRadians, targetCoordinates);
	double raHours = raRadians * (12 / M_PI);
	if (raHours < 0)
		raHours += 24.0;
	const double decDegrees = decRadians * (180 / M_PI);

	//Send the "go to" command
	QHash<QString,QString> newValues;
	newValues.insert("RA", QString::number(raHours));
	newValues.insert("DEC", QString::number(decDegrees));
	requestedPosProp->send(newValues);
}

void TelescopeClientIndi::handleDeviceDefinition(const QString& client,
                                                 const DeviceP& newDevice)
{
	if (client != indiClient->getId())
		return;
	
	if (device.isNull() && newDevice && newDevice->getName() == deviceName)
	{
		device = newDevice;
		
		connect(device.data(), SIGNAL(propertyDefined(PropertyP)),
		        this, SLOT(handlePropertyDefinition(PropertyP)));
		connect(device.data(), SIGNAL(propertyRemoved(QString)),
		        this, SLOT(handlePropertyRemoval(QString)));
		
		connect(indiClient, SIGNAL(deviceRemoved(QString,QString)),
		        this, SLOT(handleDeviceRemoval(QString,QString)));
		
		qDebug() << "Device connected:" << deviceName;
	}
}

void TelescopeClientIndi::handleDeviceRemoval(const QString& client,
                                              const QString& devName)
{
	if (!indiClient || !device)
		return;
	
	if (client != indiClient->getId() ||
	    devName != device->getName())
		return;
	
	// TODO: Something else?
	device.clear();
	qDebug() << "Device disconnected:" << deviceName;
}

void TelescopeClientIndi::handlePropertyDefinition(const PropertyP& property)
{
	if (property.isNull())
		return;
	
	QString propName = property->getName();
	qDebug() << "Property defined:" << propName;
	
	NumberPropertyP np = qSharedPointerDynamicCast<NumberProperty>(property);
	if (np.isNull())
		return;
	else
	{
		//TODO: Check permissions, too.
		//TODO: Room for improvement: detect standard property types at definition.
		if (propName == IndiClient::SP_J2000_COORDINATES ||
		    propName == IndiClient::SP_JNOW_COORDINATES)
		{
			posProp = np;
			//Make sure the intial coordinates values are handled.
			updatePositionFromProperty();
			connect(np.data(), SIGNAL(newValuesReceived()),
			        this, SLOT(updatePositionFromProperty()));
			emit coordinatesDefined(name);
		}
		else if (np->getName() == IndiClient::SP_J2000_COORDINATES_REQUEST)
		{
			requestedPosProp = np;
			isDefinedJ2000CoordinateRequest = true;
		}
		else if (np->getName() == IndiClient::SP_JNOW_COORDINATES_REQUEST)
		{
			requestedPosProp = np;
			isDefinedJNowCoordinateRequest = true;
		}
	}
	
	SwitchPropertyP sp = qSharedPointerDynamicCast<SwitchProperty>(property);
	if (sp.isNull())
		return;
	else
	{
		if (propName == IndiClient::SP_CONNECTION)
		{
			connectionProp = sp;
			connect(connectionProp.data(), SIGNAL(newValuesReceived()),
			        this, SLOT(handleConnectionEstablished()));
		}
	}
}

void TelescopeClientIndi::handlePropertyRemoval(const QString& propertyName)
{
	if (posProp && propertyName == posProp->getName())
	{
		disconnect(posProp.data(), SIGNAL(newValuesReceived()),
		        this, SLOT(updatePositionFromProperty()));
		posProp.clear();
		interpolatedPosition.reset();
		emit coordinatesUndefined(name);
	}
	else if (requestedPosProp && propertyName == requestedPosProp->getName())
	{
		requestedPosProp.clear();
		isDefinedJ2000CoordinateRequest = false;
		isDefinedJNowCoordinateRequest = false;
	}
	else if (connectionProp && propertyName == connectionProp->getName())
	{
		disconnect(connectionProp.data(), SIGNAL(newValuesReceived()),
		           this, SLOT(handleConnectionEstablished()));
		isConnectionConnected = false;
	}
}

void TelescopeClientIndi::updatePositionFromProperty()
{
	if (posProp.isNull())
		return;
	
	//Use INDI standard properties to receive location
	bool coordinatesAreInEod = false;
	
	// TODO: Again, this can be better.
	if (posProp->getName() == IndiClient::SP_JNOW_COORDINATES)
	{
		coordinatesAreInEod = true;
	}
	
	//Get the coordinates and convert them to a vector
	//TODO: Get serverTime from the property timestamp
	const qint64 serverTime = posProp->getTimestampInMilliseconds()*1000;
	const double raHours = posProp->getElement("RA")->getValue();
	const double decDegrees = posProp->getElement("DEC")->getValue();
	const double raRadians = raHours * (M_PI / 12);
	const double decRadians = decDegrees * (M_PI / 180);
	Vec3d coordinates;
	StelUtils::spheToRect(raRadians, decRadians, coordinates);
	
	Vec3d j2000Coordinates = coordinates;
	if (coordinatesAreInEod)
	{
		const StelCore* core = StelApp::getInstance().getCore();
		j2000Coordinates = core->equinoxEquToJ2000(coordinates);
	}
	
	interpolatedPosition.add(j2000Coordinates, getNow(), serverTime);
}

void TelescopeClientIndi::handleConnectionEstablished()
{
	if (connectionProp.isNull())
		return;
	
	isConnectionConnected = connectionProp->getElement(IndiClient::SP_CONNECT)->isOn();
	if (isConnectionConnected && hasQueuedGoto)
	{
		telescopeGoto(queuedGotoJ2000Pos);
		hasQueuedGoto = false;
	}
}

void TelescopeClientIndi::handleClientFailure()
{
	emit connectionLost();
}

TelescopeClientIndi* TelescopeClientIndi::telescopeClient
	(const QString& name,
	 const QString& deviceId,
	 IndiClient* indiClient, Equinox eq)
{
	TelescopeClientIndi* client = new TelescopeClientIndiPointer(name, deviceId, indiClient, eq);
	return client;
}

