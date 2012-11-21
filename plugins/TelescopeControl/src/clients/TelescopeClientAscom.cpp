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

#include "TelescopeClientAscom.hpp"

#include <cmath>

#include <QAxObject>
#include <QMetaMethod>

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"

const char* TelescopeClientAscom::P_CONNECTED = "Connected";
const char* TelescopeClientAscom::P_PARKED = "AtPark";
const char* TelescopeClientAscom::P_TRACKING = "Tracking";
const char* TelescopeClientAscom::P_CAN_SLEW = "CanSlew";
const char* TelescopeClientAscom::P_CAN_SLEW_ASYNCHRONOUSLY = "CanSlewAsync";
const char* TelescopeClientAscom::P_CAN_TRACK = "CanSetTracking";
const char* TelescopeClientAscom::P_CAN_UNPARK = "CanUnpark";
const char* TelescopeClientAscom::P_RA = "RightAscension";
const char* TelescopeClientAscom::P_DEC = "Declination";
const char* TelescopeClientAscom::P_EQUATORIAL_SYSTEM = "EquatorialSystem";

const char* TelescopeClientAscom::M_UNPARK = "Unpark()";
const char* TelescopeClientAscom::M_SLEW = "SlewToCoordinates(double, double)";


TelescopeClientAscom::TelescopeClientAscom(const QString &name, const QString &params, Equinox eq):
		TelescopeClient(name),
		equinox(eq),
		driver(0)
{
	qDebug() << "Creating ASCOM telescope client:" << name << params << flush;

	//Get the driver identifier from the parameters string
	//(for now, it contains only the driver identifier)
	driverId = params;

	//Intialize the driver object
	driver = new QAxObject(this);
	driver->setControl(driverId);
	if (driver->isNull())
		return;
	connect(driver,
	        SIGNAL(exception (int, QString, QString, QString)),
	        this,
	        SLOT(handleDriverException(int, QString, QString, QString)));
	
	// Try to connect
	driver->setProperty(P_CONNECTED, true);
	if (!driver->property(P_CONNECTED).toBool())
	{
		qDebug() << "ASCOM errror: driver failed to connect for" << name;
		deleteDriver();
	}

	//Check if the driver supports slewing to an equatorial position
	if (!canSlew())
	{
		// Not an error - this covers things like digital setting circles
		// that can only emit their current position
		qWarning() << "ASCOM warning:"
		           << name
		           << "does not accept asynchronos \"go to\" commands.";
	}
	
	// TODO: Unify TelescopeClient description messages.
	ascomDescription = driver->property("Description").toString();
	
	// Debug block
//	const QMetaObject* metaObject = driver->metaObject();
//	for(int i = 0; i < metaObject->methodCount(); ++i)
//		qDebug() << metaObject->method(i).signature();
//	qDebug() << metaObject->indexOfMethod("Dispose()");

	//If it is parked, see if it can be unparked
	//TODO: Temporary. The imporved GUI should offer parking/unparking.
//	bool isParked = driver->property(P_PARKED).toBool();
//	if (isParked)
//	{
//		if (!canUnpark())
//		{
//			qDebug() << "The" << name << "telescope is parked"
//			         << "and the Telescope control plug-in can't unpark it.";
//			deleteDriver();
//		}
//	}

	//Initialize the countdown
	timeToGetPosition = getNow() + POSITION_REFRESH_INTERVAL;
}

TelescopeClientAscom::~TelescopeClientAscom()
{
	//qDebug() << "Destructor of" << name;
	if (driver)
	{
		if (!driver->isNull())
		{
			//qDebug() << "Trying to delete...";
			//qDebug() << flush;
			if (driver->property(P_CONNECTED).toBool())
				driver->setProperty(P_CONNECTED, false);
			// FIXME: Deal with drivers that have not implemented Dispose
			//driver->dynamicCall("Dispose()");
			driver->clear();
		}
		//delete driver;
		//driver = 0;
		//qDebug() << "Deleted.";
	}
}

bool TelescopeClientAscom::isInitialized(void) const
{
	if (driver && !driver->isNull())
		return true;
	else
		return false;
}

bool TelescopeClientAscom::isConnected(void) const
{
	if (!isInitialized())
		return false;

	bool isConnected = driver->property(P_CONNECTED).toBool();
	return isConnected;
}

Vec3d TelescopeClientAscom::getJ2000EquatorialPos(const StelCore* core) const
{
	//TODO: see what to do about time_delay
	const qint64 now = getNow() - POSITION_REFRESH_INTERVAL;// - time_delay;
	return interpolatedPosition.get(now);
}

bool TelescopeClientAscom::prepareCommunication()
{
	if (!isInitialized())
		return false;

	return true;
}

void TelescopeClientAscom::performCommunication()
{
	if (!isInitialized())
		return;

	if (!isConnected())
	{
		driver->setProperty(P_CONNECTED, true);
		bool connectionAttemptSucceeded = driver->property(P_CONNECTED).toBool();
		if (!connectionAttemptSucceeded)
			return;
	}

	bool isParked = driver->property(P_PARKED).toBool();
	if (isParked)
		return;

	//Get the position every POSITION_REFRESH_INTERVAL microseconds
	const qint64 now = getNow();
	if (now < timeToGetPosition)
		return;
	else
		timeToGetPosition = now + POSITION_REFRESH_INTERVAL;

	//This returns result of type AscomInterfacesLib::EquatorialCoordinateType,
	//which is not registered. AFAIK, using it would require defining
	//the interface in Stellarium's code. I don't know how compatible is this
	//with the GNU GPL.
	/*
	//Determine the coordinate system
	//Stellarium supports only JNow (1) and J2000 (2).
	int equatorialCoordinateType = driver->property(P_EQUATORIAL_SYSTEM).toInt();
	if (equatorialCoordinateType < 1 || equatorialCoordinateType > 2)
	{
		qWarning() << "Stellarium does not support any of the coordinate "
		              "formats used by" << name;
		return;
	}*/

	//Get the coordinates and convert them to a vector
	const qint64 serverTime = getNow();
	const double raHours = driver->property(P_RA).toDouble();
	const double decDegrees = driver->property(P_DEC).toDouble();
	const double raRadians = raHours * (M_PI / 12);
	const double decRadians = decDegrees * (M_PI / 180);
	Vec3d coordinates;
	StelUtils::spheToRect(raRadians, decRadians, coordinates);

	Vec3d j2000Coordinates = coordinates;
	//See the note about equinox detection above:
	//if (equatorialCoordinateType == 1)//coordinates are in JNow
	if (equinox == EquinoxJNow)
	{
		StelCore* core = StelApp::getInstance().getCore();
		j2000Coordinates = core->equinoxEquToJ2000(coordinates);
	}

	interpolatedPosition.add(j2000Coordinates, getNow(), serverTime);
}

void TelescopeClientAscom::telescopeGoto(const Vec3d &j2000Coordinates)
{
	if (!isInitialized())
		return;

	if (!isConnected())
	{
		return;
	}

	//This returns result of type AscomInterfacesLib::EquatorialCoordinateType,
	//which is not registered. AFAIK, using it would require defining
	//the interface in Stellarium's code. I don't know how compatible is this
	//with the GNU GPL.
	/*
	//Determine the coordinate system
	//Stellarium supports only JNow (1) and J2000 (2).
	int equatorialCoordinateType = driver->property(P_EQUATORIAL_SYSTEM).toInt();
	if (equatorialCoordinateType < 1 || equatorialCoordinateType > 2)
	{
		qWarning() << "Stellarium does not support any of the coordinate "
		              "formats used by" << name;
		return;
	}
	*/

	//Parked?
//	bool isParked = driver->property(P_PARKED).toBool();
//	if (isParked)
//	{
//		bool canUnpark = driver->property(P_CAN_UNPARK).toBool();
//		if (canUnpark)
//		{
//			driver->dynamicCall(M_UNPARK);
//		}
//		else
//		{
//			qDebug() << "The" << name << "telescope is parked"
//			         << "and the Telescope control plug-in can't unpark it.";
//			return;
//		}
//	}

	//Tracking?
//	bool isTracking = driver->property(P_TRACKING).toBool();
//	if (!isTracking)
//	{
//		bool canTrack = driver->property(P_CAN_TRACK).toBool();
//		if (canTrack)
//		{
//			driver->setProperty(P_TRACKING, true);
//			isTracking = driver->property(P_TRACKING).toBool();
//			if (!isTracking)
//				return;
//		}
//		else
//		{
//			//TODO: Are there any drivers that can slew, but not track?
//			return;
//		}
//	}

	//Equatorial system
	Vec3d targetCoordinates = j2000Coordinates;
	if (equinox == EquinoxJNow)
	{
		StelCore* core = StelApp::getInstance().getCore();
		targetCoordinates = core->j2000ToEquinoxEqu(j2000Coordinates);
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
	if (canSlew())
	{
		driver->dynamicCall("SlewToCoordinatesAsync(double, double)",
		                    raHours,
		                    decDegrees);
	}
	/*
	else
	{
		//Last resort - this block the execution of Stellarium until the
		//slew is complete.
		bool canSlew = driver->property(P_CAN_SLEW).toBool();
		if (canSlew)
		{
			driver->dynamicCall(M_SLEW, raHours, decDegrees);
		}
	}
	*/
}


bool TelescopeClientAscom::canPark()
{
	return testAbility("CanPark");
}

bool TelescopeClientAscom::canSlew()
{
	return testAbility("CanSlewAsync");
}

bool TelescopeClientAscom::canSync()
{
	return testAbility("CanSync");
}

bool TelescopeClientAscom::canUnpark()
{
	return testAbility("CanUnpark");
}

void TelescopeClientAscom::abortMovement()
{
	driver->dynamicCall("AbortSlew()");
}

void TelescopeClientAscom::synch(double ra, double dec)
{
	// TODO: Finish this!
	driver->dynamicCall("SyncToCoordinates(double,double)", ra, dec);
}

void TelescopeClientAscom::handleDriverException(int code,
                                                 const QString &source,
                                                 const QString &desc,
                                                 const QString &help)
{
	Q_UNUSED(help);
	QString errorMessage = QString("%1: ASCOM driver error:\n"
	                               "Code: %2\n"
	                               "Source: %3\n"
	                               "Description: %4")
	                               .arg(name).arg(code).arg(source).arg(desc);
	qDebug() << errorMessage;
	deleteDriver();
	emit ascomError(errorMessage);
}

bool TelescopeClientAscom::testAbility(const char* name)
{
	Q_ASSERT(driver);
	return driver->property(name).toBool();
}

void TelescopeClientAscom::deleteDriver()
{
	if (driver)
	{
		if (!driver->isNull())
			driver->clear();
	}
	return;
}
