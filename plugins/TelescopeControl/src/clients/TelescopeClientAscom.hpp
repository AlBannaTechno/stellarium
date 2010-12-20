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

class TelescopeClientAscom : public TelescopeClient
{
	Q_OBJECT
public:
	TelescopeClientAscom(const QString &name, const QString &params, Equinox eq);
	virtual ~TelescopeClientAscom(void);
	bool isConnected(void) const;
	bool isInitialized(void) const;

private:
	Vec3d getJ2000EquatorialPos(const StelNavigator *nav) const;
	bool prepareCommunication();
	void performCommunication();
	void telescopeGoto(const Vec3d &j2000Pos);

	InterpolatedPosition interpolatedPosition;
	virtual bool hasKnownPosition(void) const
	{
		return interpolatedPosition.isKnown();
	}

	Equinox equinox;

	//! Qt wrapper around the ASCOM device driver COM object.
	QAxObject * driver;
	//! String identifier of the ASCOM driver object.
	QString driverId;
};

#endif //_TELESCOPE_CLIENT_ASCOM_HPP_
