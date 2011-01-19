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

#include "TelescopeClientIndi.hpp"
#include "TelescopeClientIndiLocal.hpp"
#include "TelescopeClientIndiTcp.hpp"

#include <cmath>
#include <QFile>
#include <QFileInfo>

#include "StelUtils.hpp"

TelescopeClientIndi::TelescopeClientIndi(const QString& name, Equinox eq):
	TelescopeClient(name),
	equinox(eq)
{
}


TelescopeClientIndi::~TelescopeClientIndi()
{
}

void TelescopeClientIndi::initialize()
{
	isDefinedConnection = false;
	isDefinedJ2000CoordinateRequest = false;
	isDefinedJNowCoordinateRequest = false;

	connect(&indiClient, SIGNAL(propertyDefined(QString,Property*)),
	        this, SLOT(handlePropertyDefinition(QString,Property*)));
	connect(&indiClient, SIGNAL(propertyUpdated(QString,Property*)),
	        this, SLOT(handlePropertyUpdate(QString,Property*)));
	
	//TODO: Find a better way
	//TODO: Move to another function, check if this property has been changed.
	QString commandConnect = "<newSwitchVector device= \"Telescope Simulator\" name=\"CONNECTION\">\n<oneSwitch name=\"CONNECT\">On</oneSwitch>\n<oneSwitch name=\"DISCONNECT\">Off</oneSwitch>\n</newSwitchVector>";
	indiClient.sendRawCommand(commandConnect);
	
}

Vec3d TelescopeClientIndi::getJ2000EquatorialPos(const StelNavigator *nav) const
{
	Q_UNUSED(nav);
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
		//
	}

	//TODO: Verify what kind of action is going to be performed on a request

	Vec3d targetCoordinates;
	QString command;
	if (isDefinedJ2000CoordinateRequest)
	{
		targetCoordinates = j2000Coordinates;
		command = IndiClient::SP_J2000_COORDINATES_REQUEST;
	}
	else if (isDefinedJNowCoordinateRequest)
	{
		const StelNavigator* navigator = StelApp::getInstance().getCore()->getNavigator();
		targetCoordinates = navigator->equinoxEquToJ2000(j2000Coordinates);
		command = IndiClient::SP_JNOW_COORDINATES_REQUEST;
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
	//TODO: Again, this is not the right way. :)
	QString xmlElement = "<newNumberVector device= \"Telescope Simulator\" name=\"%3\">\n<oneNumber name=\"RA\">%1</oneNumber>\n<oneNumber name=\"DEC\">%2</oneNumber>\n</newNumberVector>";
	xmlElement = xmlElement.arg(raHours).arg(decDegrees).arg(command);
	indiClient.sendRawCommand(xmlElement);
}

void TelescopeClientIndi::handlePropertyDefinition(const QString& device, Property* property)
{
	Q_UNUSED(device);
	//TODO: Check for IndiClient::SP_CONNECTION
	NumberProperty* numberProperty = dynamic_cast<NumberProperty*>(property);
	if (numberProperty)
	{
		//TODO: Check permissions, too.
		if (numberProperty->getName() == IndiClient::SP_J2000_COORDINATES_REQUEST)
		{
			isDefinedJ2000CoordinateRequest = true;
		}
		else if (numberProperty->getName() == IndiClient::SP_JNOW_COORDINATES_REQUEST)
		{
			isDefinedJNowCoordinateRequest = true;
		}
	}
}

void TelescopeClientIndi::handlePropertyUpdate(const QString& device, Property* property)
{
	Q_UNUSED(device);
	NumberProperty* numberProperty = dynamic_cast<NumberProperty*>(property);
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
				const StelNavigator* navigator = StelApp::getInstance().getCore()->getNavigator();
				j2000Coordinates = navigator->equinoxEquToJ2000(coordinates);
			}

			interpolatedPosition.add(j2000Coordinates, getNow(), serverTime);
		}
	}
}

TelescopeClientIndi* TelescopeClientIndi::telescopeClient(const QString& name, const QString& driverName, Equinox eq)
{
	TelescopeClientIndi* client = new TelescopeClientIndiLocal(name, driverName, eq);
	client->initialize();
	return client;
}

TelescopeClientIndi* TelescopeClientIndi::telescopeClient(const QString& name, const QString& host, int port, Equinox eq)
{
	TelescopeClientIndi* client = new TelescopeClientIndiTcp(name, host, port, eq);
	client->initialize();
	return client;
}

