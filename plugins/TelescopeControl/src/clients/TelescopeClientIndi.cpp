/*
 * Stellarium Telescope Control plug-in
 * Copyright (C) 2010-2011  Bogdan Marinov
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

#include "TelescopeClientIndi.hpp"
#include "TelescopeClientIndiLocal.hpp"
#include "TelescopeClientIndiTcp.hpp"
#include "TelescopeClientIndiPointer.hpp"

#include <cmath>
#include "StelUtils.hpp"

TelescopeClientIndi::TelescopeClientIndi(const QString& name, Equinox eq):
	TelescopeClient(name),
	equinox(eq),
	indiClient(0)
{
}

TelescopeClientIndi::~TelescopeClientIndi()
{
}

void TelescopeClientIndi::initialize()
{
	isDefinedConnection = false;
	isConnectionConnected = false;
	isDefinedJ2000CoordinateRequest = false;
	isDefinedJNowCoordinateRequest = false;
	hasQueuedGoto = false;

	connect(indiClient, SIGNAL(propertyDefined(QString,PropertyP)),
	        this, SLOT(handlePropertyDefinition(QString,PropertyP)));
	connect(indiClient, SIGNAL(propertyUpdated(QString,PropertyP)),
	        this, SLOT(handlePropertyUpdate(QString,PropertyP)));
}

IndiClient* TelescopeClientIndi::getIndiClient() const
{
	return indiClient;
}

Vec3d TelescopeClientIndi::getJ2000EquatorialPos(const StelCore *core) const
{
	Q_UNUSED(core);
	//TODO: see what to do about time_delay
	const qint64 now = getNow() - 500000;// - time_delay;
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

	//If it's not connected, attempt to connect
	if (isDefinedConnection && !isConnectionConnected)
	{
		hasQueuedGoto = true;
		queuedGotoJ2000Pos = j2000Coordinates;
		QVariantHash elements;
		elements.insert(IndiClient::SP_CONNECT, true);
		elements.insert(IndiClient::SP_DISCONNECT, false);
		indiClient->writeProperty(deviceName, IndiClient::SP_CONNECTION, elements);
		//(isDefinedConnection == true ensures that deviceName is not empty)
		return;
	}

	//TODO: Verify what kind of action (SLEW, SYNC) is going to be performed on a request

	Vec3d targetCoordinates;
	QString property;
	if (isDefinedJ2000CoordinateRequest)
	{
		targetCoordinates = j2000Coordinates;
		property = IndiClient::SP_J2000_COORDINATES_REQUEST;
	}
	else if (isDefinedJNowCoordinateRequest)
	{
		const StelCore* core = StelApp::getInstance().getCore();
		targetCoordinates = core->j2000ToEquinoxEqu(j2000Coordinates);
		property = IndiClient::SP_JNOW_COORDINATES_REQUEST;
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
	QVariantHash newValues;
	newValues.insert("RA", raHours);
	newValues.insert("DEC", decDegrees);
	indiClient->writeProperty(deviceName, property, newValues);
}

void TelescopeClientIndi::handlePropertyDefinition(const QString& device, const PropertyP& property)
{
	//Ignore this property if it belongs to another device.
	if (device != deviceName)
		return;

	NumberPropertyP numberProperty = qSharedPointerDynamicCast<NumberProperty>(property);
	if (numberProperty.isNull())
		return;
	else
	{
		//TODO: Check permissions, too.
		if (numberProperty->getName() == IndiClient::SP_J2000_COORDINATES)
		{
			//Make sure the intial coordinates values are handled.
			handlePropertyUpdate(device, property);
		}
		else if (numberProperty->getName() == IndiClient::SP_JNOW_COORDINATES)
		{
			//Make sure the intial coordinates values are handled.
			handlePropertyUpdate(device, property);
		}
		else if (numberProperty->getName() == IndiClient::SP_J2000_COORDINATES_REQUEST)
		{
			isDefinedJ2000CoordinateRequest = true;
		}
		else if (numberProperty->getName() == IndiClient::SP_JNOW_COORDINATES_REQUEST)
		{
			isDefinedJNowCoordinateRequest = true;
		}
	}
	
	SwitchPropertyP switchProperty = qSharedPointerDynamicCast<SwitchProperty>(property);
	if (switchProperty.isNull())
		return;
	else
	{
		if (switchProperty->getName() == IndiClient::SP_CONNECTION)
		{
			isDefinedConnection = true;
		}
	}
}

void TelescopeClientIndi::handlePropertyUpdate(const QString& device, const PropertyP& property)
{
	//Ignore this property if it belongs to another device.
	if (deviceName != device)
		return;

	NumberPropertyP numberProperty = qSharedPointerDynamicCast<NumberProperty>(property);
	if (numberProperty)
	{
		//Use INDI standard properties to receive location
		bool hasReceivedCoordinates = false;
		bool coordinatesAreInEod = false;

		if (numberProperty->getName() == IndiClient::SP_J2000_COORDINATES)
		{
			hasReceivedCoordinates = true;
		}
		else if (numberProperty->getName() == IndiClient::SP_JNOW_COORDINATES)
		{
			hasReceivedCoordinates = true;
			coordinatesAreInEod = true;
		}

		if (hasReceivedCoordinates)
		{
			//Get the coordinates and convert them to a vector
			//TODO: Get serverTime from the property timestamp
			const qint64 serverTime = getNow();
			const double raHours = numberProperty->getElement("RA")->getValue();
			const double decDegrees = numberProperty->getElement("DEC")->getValue();
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

		return;
	}

	SwitchPropertyP switchProperty = qSharedPointerDynamicCast<SwitchProperty>(property);
	if (switchProperty)
	{
		if (isDefinedConnection && switchProperty->getName() == IndiClient::SP_CONNECTION)
		{
			isConnectionConnected = switchProperty->getElement(IndiClient::SP_CONNECT)->isOn();
			if (isConnectionConnected && hasQueuedGoto)
			{
				telescopeGoto(queuedGotoJ2000Pos);
				hasQueuedGoto = false;
			}
		}
	}
}

TelescopeClientIndi* TelescopeClientIndi::telescopeClient(const QString& name, const QString& driverName, Equinox eq)
{
	TelescopeClientIndi* client = new TelescopeClientIndiLocal(name, driverName, eq);
	return client;
}

TelescopeClientIndi* TelescopeClientIndi::telescopeClient(const QString& name, const QString& host, int port, Equinox eq)
{
	TelescopeClientIndi* client = new TelescopeClientIndiTcp(name, host, port, eq);
	return client;
}

TelescopeClientIndi* TelescopeClientIndi::telescopeClient
	(const QString& name,
	 const QString& deviceId,
	 IndiClient* indiClient,
	 Equinox eq)
{
	TelescopeClientIndi* client = new TelescopeClientIndiPointer(name, deviceId, indiClient, eq);
	return client;
}

