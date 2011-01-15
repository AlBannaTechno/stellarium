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

#include "StelUtils.hpp"

TelescopeClientIndi::TelescopeClientIndi(const QString& name, const QString& params, Equinox eq):
		TelescopeClient(name),
		equinox(eq),
		tcpSocket(0),
		driverProcess(0)
{
	qDebug() << "Creating INDI telescope client:" << name << params;

	//TODO: Actually parse the parameters
	//TODO: For now, for testing purposes, all connections are remote
	isRemoteConnection = true;

	if (isRemoteConnection)
	{
		tcpSocket = new QTcpSocket();
		for (int i=0; i<3; i++)
		{
			tcpSocket->connectToHost("localhost", 7624);
			if (tcpSocket->waitForConnected(1000))
			{
				connect(tcpSocket,
				        SIGNAL(error(QAbstractSocket::SocketError)),
				        this,
				        SLOT(handleConnectionError(QTcpSocket::SocketError)));
				indiClient.addConnection(tcpSocket);
				break;
			}
		}
	}
	else
	{
		driverProcess = new QProcess(this);
		//QString driverPath = "/home/bogdan/indi/qtcreator-build/tutorial_two";
		QString driverPath = "indi_lx200classic";
		QStringList driverArguments;
		//driverArguments << "\"Test Driver\"";
		//driverArguments << "/dev/ttyS0";
		driverProcess->start(driverPath, driverArguments);
		connect(driverProcess, SIGNAL(error(QProcess::ProcessError)),
		        this, SLOT(handleDriverError(QProcess::ProcessError)));
		indiClient.addConnection(driverProcess);
	}

	connect(&indiClient, SIGNAL(propertyUpdated(QString,Property*)),
	        this, SLOT(handlePropertyUpdate(QString,Property*)));
}

TelescopeClientIndi::~TelescopeClientIndi(void)
{
	//TODO: Disconnect stuff
	if (tcpSocket)
	{
		disconnect(tcpSocket,
		           SIGNAL(error(QTcpSocket::SocketError)),
		           this,
		           SLOT(handleConnectionError(QTcpSocket::SocketError)));
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

bool TelescopeClientIndi::isInitialized(void) const
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

bool TelescopeClientIndi::isConnected(void) const
{
	if (!isInitialized())
		return false;
	else
		return true;
}

Vec3d TelescopeClientIndi::getJ2000EquatorialPos(const StelNavigator *nav) const
{
	//TODO: see what to do about time_delay
	const qint64 now = getNow() - POSITION_REFRESH_INTERVAL;// - time_delay;
	return interpolatedPosition.get(now);
}

bool TelescopeClientIndi::prepareCommunication()
{
	if (!isInitialized())
		return false;

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

	//Equatorial system
	Vec3d targetCoordinates = j2000Coordinates;
	//See the note about equinox detection above:
	//if (equatorialCoordinateType == 1)//coordinates are in JNow
	if (equinox == EquinoxJNow)
	{
		const StelNavigator* navigator = StelApp::getInstance().getCore()->getNavigator();
		targetCoordinates = navigator->equinoxEquToJ2000(j2000Coordinates);
	}

	//Convert coordinates from the vector
	double raRadians;
	double decRadians;
	StelUtils::rectToSphe(&raRadians, &decRadians, targetCoordinates);
	double raHours = raRadians * (12 / M_PI);
	if (raHours < 0)
		raHours += 24.0;
	const double decDegrees = decRadians * (180 / M_PI);

	//Send the "go to" command
	//
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

void TelescopeClientIndi::handlePropertyUpdate(QString device, Property* property)
{
	NumberProperty* numberProperty = dynamic_cast<NumberProperty*>(property);
	if (numberProperty)
	{
		if (numberProperty->getName() == IndiClient::SP_JNOW_COORD)
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
			//As the coordinates in this standard property are always in EOD:
			//if (equinox == EquinoxJNow)
			{
				const StelNavigator* navigator = StelApp::getInstance().getCore()->getNavigator();
				j2000Coordinates = navigator->equinoxEquToJ2000(coordinates);
			}

			interpolatedPosition.add(j2000Coordinates, getNow(), serverTime);
		}
	}
}
