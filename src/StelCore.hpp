/*
 * Copyright (C) 2003 Fabien Chereau
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

#ifndef _STEL_CORE_H_
#define _STEL_CORE_H_

#include <string>

#include "callbacks.hpp"
#include "Navigator.hpp"
#include "Projector.hpp"
#include "ToneReproducer.hpp"

class Observer;

//!  @brief Main class for stellarium core processing.
//!
//! Manage all the base modules which must be present in stellarium
class StelCore
{
public:
	// Inputs are the locale directory and root directory and callback function for recording actions
    StelCore(const boost::callback <void, string> & recordCallback);
    virtual ~StelCore();

	//! Init and load all main core components from the passed config file.
	void init(const InitParser& conf, LoadingBar& lb);

	//! Init projection temp TODO remove
	void initProj(const InitParser& conf);

	//! Update all the objects with respect to the time.
	//! @param delta_time the time increment in ms.
	void update(int delta_time);

	//! Execute all the drawing functions
	//! @param delta_time the time increment in ms.
	//! @returns the max squared distance in pixels any single object has moved since the previous update.
	double draw(int delta_time);
	
	//! Get the current projector used in the core
	Projector* getProjection() {return projection;}
	const Projector* getProjection() const {return projection;}
	
	//! Get the current navigation (manages frame transformation) used in the core
	Navigator* getNavigation() {return navigation;}
	const Navigator* getNavigation() const {return navigation;}

	//! Get the current tone converter used in the core
	ToneReproducer* getToneReproducer() {return tone_converter;}
	const ToneReproducer* getToneReproducer() const {return tone_converter;}

	///////////////////////////////////////////////////////////////////////////////////////
	// Planets flags
	bool setHomePlanet(string planet);

	///////////////////////////////////////////////////////////////////////////////////////
	// Projection
	//! Set the projection type
	void setProjectionType(const string& ptype);
	//! Get the projection type
	string getProjectionType(void) const {return Projector::typeToString(projection->getType());}


	///////////////////////////////////////////////////////////////////////////////////////
	// Observer
	//! Return the current observatory (as a const object)
	const Observer* getObservatory(void) {return observatory;}

	//! Move to a new latitude and longitude on home planet
	void moveObserver(double lat, double lon, double alt, int delay, const wstring& name)
	{
		observatory->moveTo(lat, lon, alt, delay, name);
	}

	///////////////////////////////////////////////////////////////////////////////////////
	// Others
	//! Load color scheme from the given ini file and section name
	void setColorScheme(const string& skinFile, const string& section);

private:
	//! Callback to record actions
	boost::callback<void, string> recordActionCallback;
		
	// Main elements of the program
	Navigator * navigation;				// Manage all navigation parameters, coordinate transformations etc..
	Observer * observatory;			// Manage observer position
	Projector * projection;				// Manage the projection mode and matrix
	ToneReproducer * tone_converter;	// Tones conversion between stellarium world and display device
	class MovementMgr* movementMgr;		// Manage vision movements
	
	class HipStarMgr * hip_stars;		// Manage the hipparcos stars
	class ConstellationMgr * asterisms;		// Manage constellations (boundaries, names etc..)
	class NebulaMgr * nebulas;				// Manage the nebulas
	class SolarSystem* ssystem;				// Manage the solar system
	class MilkyWay* milky_way;				// Our galaxy
	class MeteorMgr* meteors;				// Manage meteor showers
	class ImageMgr* script_images;           // for script loaded image display
	class TelescopeMgr *telescope_mgr;
	class LandscapeMgr* landscape;
	class GeodesicGridDrawer* geoDrawer;
	class GridLinesMgr* gridLines;
};

#endif // _STEL_CORE_H_
