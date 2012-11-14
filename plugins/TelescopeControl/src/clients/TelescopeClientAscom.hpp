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

#ifndef _TELESCOPE_CLIENT_ASCOM_HPP_
#define _TELESCOPE_CLIENT_ASCOM_HPP_

#include "TelescopeClient.hpp"
#include "InterpolatedPosition.hpp"

#include <QAxObject>
#include <QObject>
#include <QString>

//! Telescope client that uses an ASCOM driver.
//! The ASCOM platform is a driver interface standard based on Microsoft's
//! Component Object Model (COM). ASCOM drivers are COM objects, so this class
//! has to use Qt's AxContainer module. In this class, the selected driver is
//! loaded by a QAxObject object, and communication with it (the driver) is
//! possible only via QAxObject::dynamicCall() and similar functions.
class TelescopeClientAscom : public TelescopeClient
{
	Q_OBJECT
public:
	TelescopeClientAscom(const QString &name, const QString &params, Equinox eq);
	virtual ~TelescopeClientAscom();
	bool isConnected(void) const;
	bool isInitialized(void) const;

	//! Estimates a current position from the stored previous positions.
	//! InterpolatedPosition is used to make the apparent movement of the
	//! telescope reticle smoother.
	Vec3d getJ2000EquatorialPos(const StelCore* core) const;
	bool prepareCommunication();
	void performCommunication();
	void telescopeGoto(const Vec3d &j2000Pos);

signals:
	void ascomError(QString);

private slots:
	void handleDriverException(int code,
	                           const QString& source,
	                           const QString& desc,
	                           const QString& help);

private:
	InterpolatedPosition interpolatedPosition;
	virtual bool hasKnownPosition(void) const
	{
		return interpolatedPosition.isKnown();
	}

	Equinox equinox;

	//! Qt wrapper around the ASCOM device driver COM object.
	QAxObject* driver;
	//! String identifier of the ASCOM driver object.
	QString driverId;

	//! Deletes the internal driver object, putting the client into
	//! a deinitialized state.
	void deleteDriver();

	//! Counter for taking the telescope's position every
	//! POSITION_REFRESH_INTERVAL microseconds.
	qint64 timeToGetPosition;
	//! Interval between attempts to read position info, in microseconds.
	//! Default is half a second.
	static const qint64 POSITION_REFRESH_INTERVAL = 500000;
	
	//! ASCOM driver description extracted from the driver object.
	QString ascomDescription;

	//ASCOM named properties and functions
	static const char* P_CONNECTED;
	static const char* P_PARKED;
	static const char* P_TRACKING;
	static const char* P_CAN_SLEW;
	static const char* P_CAN_SLEW_ASYNCHRONOUSLY;
	static const char* P_CAN_TRACK;
	static const char* P_CAN_UNPARK;
	static const char* P_RA;
	static const char* P_DEC;
	static const char* P_EQUATORIAL_SYSTEM;
	static const char* M_UNPARK;
	static const char* M_SLEW;
	static const char* M_SLEW_ASYNCHRONOUSLY;
};

#endif //_TELESCOPE_CLIENT_ASCOM_HPP_
