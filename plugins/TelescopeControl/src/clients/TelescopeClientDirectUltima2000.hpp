/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2009-2011 Bogdan Marinov (this file,
 * reusing code written by Johannes Gajdosik in 2006)
 * 
 * Johannes Gajdosik wrote in 2006 the original telescope control feature
 * as a core module of Stellarium. In 2009 it was significantly extended with
 * GUI features and later split as an external plug-in module by Bogdan Marinov.
 * 
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

#ifndef _TELESCOPE_CLIENT_DIRECT_ULTIMA_2000_
#define _TELESCOPE_CLIENT_DIRECT_ULTIMA_2000_

#include <QObject>
#include <QString>

#include "StelApp.hpp"
#include "StelObject.hpp"
#include "StelCore.hpp"

#include "Server.hpp" //from the telescope server source tree
#include "TelescopeClient.hpp" //from the plug-in's source tree
#include "InterpolatedPosition.hpp"

class Ultima2000Connection;

//! Telescope client that connects directly to a Celestron Ultima 2000 through
//! a serial port. This class has been created by reusing code from
//! TelescopeClientDirectNexStar.
class TelescopeClientDirectUltima2000 : public TelescopeClient, public Server
{
	Q_OBJECT
public:
	TelescopeClientDirectUltima2000(const QString &name, const QString &parameters, Equinox eq = EquinoxJ2000);
	~TelescopeClientDirectUltima2000()
	{
		//hangup();
	}
	
	//======================================================================
	// Methods inherited from TelescopeClient
	bool isConnected() const;
	
	//======================================================================
	// Methods inherited from Server
	virtual void step(long long int timeout_micros);
	void communicationResetReceived();
	void raDecReceived(unsigned int ra_int, unsigned int dec_int);
	
private:
	//======================================================================
	// Methods inherited from TelescopeClient
	Vec3d getJ2000EquatorialPos(const StelCore* nav=0) const;
	bool prepareCommunication();
	void performCommunication();
	void telescopeGoto(const Vec3d &j2000Pos);
	bool isInitialized() const;
	
	//======================================================================
	// Methods inherited from Server
	void sendPosition(unsigned int ra_int, int dec_int, int status);
	//Inherited from pure virtual Server::gotoReceived(), so the parameters
	//can't be changed.
	void gotoReceived(unsigned int ra_int, int dec_int);
	
private:
	void hangup();
	int time_delay;
	
	InterpolatedPosition interpolatedPosition;
	virtual bool hasKnownPosition() const
	{
		return interpolatedPosition.isKnown();
	}

	Equinox equinox;
	
	Ultima2000Connection *ultima2000;
	
	bool queue_get_position;
	long long int next_pos_time;
};

#endif //_TELESCOPE_CLIENT_DIRECT_ULTIMA_2000_
