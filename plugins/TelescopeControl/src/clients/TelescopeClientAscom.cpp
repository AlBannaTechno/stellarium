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

#include <QAxObject>

TelescopeClientAscom::TelescopeClientAscom(const QString &name, const QString &params, Equinox eq):
		TelescopeClient(name),
		equinox(eq)
{
	driver = new QAxObject(this);
}

TelescopeClientAscom::~TelescopeClientAscom(void)
{
	//
}

bool TelescopeClientAscom::isConnected(void) const
{
	//
}

Vec3d TelescopeClientAscom::getJ2000EquatorialPos(const StelNavigator *nav) const
{
	//
}

bool TelescopeClientAscom::prepareCommunication()
{
	//
}

void TelescopeClientAscom::performCommunication()
{
	//
}

void TelescopeClientAscom::telescopeGoto(const Vec3d &j2000Pos)
{
	//
}

bool TelescopeClientAscom::isInitialized(void) const
{
	//
}
