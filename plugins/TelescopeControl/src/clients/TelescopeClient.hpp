/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2006 Johannes Gajdosik
 * Copyright (C) 2009-2011 Bogdan Marinov
 * 
 * This module was originally written by Johannes Gajdosik in 2006
 * as a core module of Stellarium. In 2009 it was significantly extended with
 * GUI features and later split as an external plug-in module by Bogdan Marinov.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _TELESCOPE_CLIENT_HPP_
#define _TELESCOPE_CLIENT_HPP_

#include <QList>
#include <QString>
#include <QObject>

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelObject.hpp"
#include "InterpolatedPosition.hpp"

qint64 getNow(void);

enum Equinox {
	EquinoxJ2000,
	EquinoxJNow
};

//! An abstract base class that should never be used directly, only inherited.
//! This class used to be called Telescope, but it has been renamed
//! to TelescopeClient in order to resolve a compiler/linker conflict
//! with the identically named Telescope class in Stellarium's main code.
class TelescopeClient : public QObject, public StelObject
{
	Q_OBJECT
public:
	virtual ~TelescopeClient(void) {}
	
	// Method inherited from StelObject
	QString getEnglishName(void) const {return name;}
	QString getNameI18n(void) const {return nameI18n;}
	Vec3f getInfoColor(void) const
	{
		return StelApp::getInstance().getVisionModeNight() ? Vec3f(0.8, 0.2, 0.2) : Vec3f(1, 1, 1);
	}
	//! TelescopeClient supports the following InfoStringGroup flags:
	//! - Name
	//! - RaDecJ2000
	//! - RaDec
	//! - PlainText
	//! @param core the StelCore object
	//! @param flags a set of InfoStringGroup items to include in the return value.
	//! @return a QString containing an HMTL encoded description of the Telescope.
	QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const;
	QString getType(void) const {return "Telescope";}
	virtual double getAngularSize(const StelCore*) const {Q_ASSERT(0); return 0;}	// TODO
		
	// Methods specific to telescope
	virtual void telescopeGoto(const Vec3d &j2000Pos) = 0;
	virtual bool isConnected(void) const = 0;
	virtual bool isInitialized(void) const {return true;}
	virtual bool hasKnownPosition(void) const = 0;
	void addOcular(double fov) {if (fov>=0.0) oculars.push_back(fov);}
	const QList<double> &getOculars(void) const {return oculars;}
	
	virtual bool prepareCommunication() {return false;}
	virtual void performCommunication() {}

protected:
	TelescopeClient(const QString &name);
	QString nameI18n;
	const QString name;
private:
	float getSelectPriority(const StelCore*) const {return -10.f;}
private:
	QList<double> oculars; // fov of the oculars
};

#endif // _TELESCOPE_CLIENT_HPP_
