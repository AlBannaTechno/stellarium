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

#ifndef _TELESCOPE_CLIENT_INDI_POINTER_HPP_
#define _TELESCOPE_CLIENT_INDI_POINTER_HPP_

#include "TelescopeClientIndi.hpp"

#include <QObject>

//! A pointing device controlled using the INDI protocol.
//! A sub-connection of a TelescopeClientIndiLocal or TelescopeClientIndiTcp.
//! Unlike them, it actually represents a device and displays a reticle on the
//! screen like the rest of the telescope clients.
class TelescopeClientIndiPointer : public TelescopeClientIndi
{
	Q_OBJECT
public:
	TelescopeClientIndiPointer(const QString& name,
	                           const QString& deviceId,
	                           IndiClient* client,
	                           Equinox eq);
	~TelescopeClientIndiPointer();
	//! \returns true if the standard connection switch is On.
	bool isConnected() const;
	bool isInitialized() const;

protected:
	bool prepareCommunication();
	void performCommunication();
};

#endif //_TELESCOPE_CLIENT_INDI_POINTER_HPP_
