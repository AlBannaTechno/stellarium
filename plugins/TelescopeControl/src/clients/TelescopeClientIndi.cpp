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

#include "TelescopeClientIndi.hpp"

#include <cmath>
#include <QFile>
#include <QFileInfo>

#include "StelUtils.hpp"

TelescopeClientIndi::TelescopeClientIndi(const QString& name, const QString& host, int port, Equinox eq):
	TelescopeClient(name), 
	equinox(eq),
	tcpSocket(0),
	driverProcess(0)
{
	qDebug() << "Creating INDI telescope client:" << name;
	
	if (host.isEmpty())
		return;
	//TODO: Validation
	if (port <= 0)
		return;
	
	tcpSocket = new QTcpSocket();
	for (int i=0; i<3; i++)
	{
		tcpSocket->connectToHost(host, port);
		if (tcpSocket->waitForConnected(1000))
		{
			connect(tcpSocket,
					  SIGNAL(error(QAbstractSocket::SocketError)),
					  this,
					  SLOT(handleConnectionError(QAbstractSocket::SocketError)));
			indiClient.addConnection(tcpSocket);
			break;
		}
	}
}

TelescopeClientIndi::TelescopeClientIndi(const QString& name, const QString& driverName, Equinox eq):
	TelescopeClient(name), 
	equinox(eq),
	tcpSocket(0),
	driverProcess(0)
{
	qDebug() << "Creating INDI telescope client:" << name;
	
	//TODO: Again, use exception instead of this.
	if (driverName.isEmpty()
		 || !QFile::exists("/usr/bin/" + driverName)
		 || !QFileInfo("/usr/bin/" + driverName).isExecutable())
		return;
	
	driverProcess = new QProcess(this);
	//TODO: Fix this!
	//TODO: Pass the full path instead of only the filename?
	QString driverPath = "/usr/bin/" + driverName;
	QStringList driverArguments;
	qDebug() << driverPath;
	driverProcess->start(driverPath, driverArguments);
	connect(driverProcess, SIGNAL(error(QProcess::ProcessError)),
			  this, SLOT(handleDriverError(QProcess::ProcessError)));
	indiClient.addConnection(driverProcess);
}

TelescopeClientIndi::~TelescopeClientIndi()
{
	//TODO: Disconnect stuff
	if (tcpSocket)
	{
		disconnect(tcpSocket,
		           SIGNAL(error(QAbstractSocket::SocketError)),
		           this,
		           SLOT(handleConnectionError(QAbstractSocket::SocketError)));
		tcpSocket->disconnectFromHost();
		tcpSocket->deleteLater();
	}

	if (driverProcess)
	{
		disconnect(driverProcess, SIGNAL(error(QProcess::ProcessError)),
		           this, SLOT(handleDriverError(QProcess::ProcessError)));
		//TODO: There was some problems on Windows with QProcess - one of the
		//termination methods didn't work. This is not a problem at the moment.
		driverProcess->terminate();
		driverProcess->waitForFinished();
		driverProcess->deleteLater();
	}
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

bool TelescopeClientIndi::isInitialized() const
{
	//TODO: Improve the checks.
	if (isRemoteConnection)
	{
		if (tcpSocket->isOpen() &&
		    tcpSocket->isReadable() &&
		    tcpSocket->isWritable())
			return true;
		else
			return false;
	}
	else
	{
		if (driverProcess->state() == QProcess::Running &&
		    driverProcess->isOpen())
			return true;
		else
			return false;
	}
}

bool TelescopeClientIndi::isConnected() const
{
	//TODO: Should include a check for the CONNECTION property
	if (!isInitialized())
		return false;
	else
		return true;
}

Vec3d TelescopeClientIndi::getJ2000EquatorialPos(const StelNavigator *nav) const
{
	Q_UNUSED(nav);
	//TODO: see what to do about time_delay
	const qint64 now = getNow() - 500000;// - time_delay;
	return interpolatedPosition.get(now);
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

void TelescopeClientIndi::handleConnectionError(QTcpSocket::SocketError error)
{
	//TODO
	qDebug() << error;
	qDebug() << tcpSocket->errorString();
}

void TelescopeClientIndi::handleDriverError(QProcess::ProcessError error)
{
	//TODO
	qDebug() << error;
	qDebug() << driverProcess->errorString();
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
