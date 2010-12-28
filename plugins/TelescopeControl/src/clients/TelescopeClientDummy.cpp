/*
 * Stellarium Telescope Control plug-in
 * 
 * Copyright (C) 2006 Johannes Gajdosik
 * Copyright (C) 2009-2010 Bogdan Marinov
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "TelescopeClientDummy.hpp"

TelescopeClientDummy::TelescopeClientDummy(const QString& name, const QString&) :
		TelescopeClient(name)
{
	desired_pos[0] = XYZ[0] = 1.0;
	desired_pos[1] = XYZ[1] = 0.0;
	desired_pos[2] = XYZ[2] = 0.0;
}

bool TelescopeClientDummy::isConnected() const
{
	return true;
}

bool TelescopeClientDummy::prepareCommunication()
{
	XYZ = XYZ * 31.0 + desired_pos;
	const double lq = XYZ.lengthSquared();
	if (lq > 0.0)
		XYZ *= (1.0/sqrt(lq));
	else
		XYZ = desired_pos;
	return true;
}

void TelescopeClientDummy::telescopeGoto(const Vec3d& j2000Pos)
{
	desired_pos = j2000Pos;
	desired_pos.normalize();
}

bool TelescopeClientDummy::hasKnownPosition() const
{
	return true;
}

Vec3d TelescopeClientDummy::getJ2000EquatorialPos(const StelNavigator*) const
{
	return XYZ;
}
