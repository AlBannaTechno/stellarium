/*
 * Stellarium Telescope Control plug-in
 * Copyright (C) 2010-2011  Bogdan Marinov
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

#include "TelescopeClientIndiPointer.hpp"

TelescopeClientIndiPointer::TelescopeClientIndiPointer(const QString& name,
                                                       const QString& deviceId,
                                                       IndiClient* client,
                                                       Equinox eq)
	: TelescopeClientIndi(name, eq)
{
	Q_ASSERT(client); //TODO: Better check. Throw something?
	deviceName = deviceId;
	initialize();
	client->writeGetProperties(deviceId);
}

TelescopeClientIndiPointer::~TelescopeClientIndiPointer()
{
	//
}

bool TelescopeClientIndiPointer::isInitialized() const
{
	if (indiClient && !deviceName.isEmpty())
		return true;
	else
		return false;
}

bool TelescopeClientIndiPointer::isConnected() const
{
	//TODO: Is this a good idea?
	return isConnectionConnected;
}

bool TelescopeClientIndiPointer::prepareCommunication()
{
	//TODO: Perhaps I can use this???
	return true;
}

void TelescopeClientIndiPointer::performCommunication()
{
	//
}




