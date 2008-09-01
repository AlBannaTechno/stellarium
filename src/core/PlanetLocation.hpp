/*
 * Copyright (C) 2008 Fabien Chereau
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

#ifndef _PLANET_LOCATION_HPP_
#define _PLANET_LOCATION_HPP_

#include <QString>

//! @class PlanetLocation
//! Store the informations for a location on a planet
class PlanetLocation
{
public:
	PlanetLocation();
	//! Return a short string which can be used in a list view
	QString toSmallString() const;
			
	//! Location/city name
	QString name;
	//! English country name or empty string
	QString country;
	//! State/region name
	QString state;
	//! English planet name
	QString planetName;
	//! Longitude in degree
	double longitude;
	//! Latitude in degree
	double latitude;
	//! Altitude in meter
	int altitude;
	//! A hint for associating a landscape to the location
	QString landscapeKey;
	
private:
	mutable QString cachePlanetNameI18n;
};

#endif // _PLANET_LOCATION_HPP_
