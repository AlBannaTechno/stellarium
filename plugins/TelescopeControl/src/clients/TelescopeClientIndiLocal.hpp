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

#ifndef _TELESCOPE_CLIENT_INDI_LOCAL_HPP_
#define _TELESCOPE_CLIENT_INDI_LOCAL_HPP_

#include "TelescopeClientIndi.hpp"
#include "InterpolatedPosition.hpp"
#include "IndiClient.hpp"

#include <QObject>
#include <QProcess>
#include <QString>

//! Telescope client that uses the INDI protocol.
//! This class is a specialization of the TelescopeClientIndi class, that starts a local INDI
//! driver for its communication medium.
class TelescopeClientIndiLocal : public TelescopeClientIndi
{
	Q_OBJECT
public:
	TelescopeClientIndiLocal(const QString& name, const QString& driverName, Equinox eq);
	virtual ~TelescopeClientIndiLocal();
	bool isConnected() const;
	bool isInitialized() const;

protected:
	virtual bool prepareCommunication();
	virtual void performCommunication();

protected slots:
	void handleDriverError(QProcess::ProcessError error);

private:
	QProcess* driverProcess;
};

#endif //_TELESCOPE_CLIENT_INDI_LOCAL_HPP_
