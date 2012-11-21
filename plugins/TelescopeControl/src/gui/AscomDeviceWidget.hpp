/*
 * Stellarium Device Control plug-in
 * Copyright (C) 2012  Bogdan Marinov
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
 * Foundation, Inc., 51 Franklin Street,
 * Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef ASCOMDEVICEWIDGET_HPP
#define ASCOMDEVICEWIDGET_HPP

#include <QWidget>

#include "TelescopeClientAscom.hpp"

class CoordinatesWidget;
class FovCirclesWidget;
class TelescopeControl;

//! Control panel tab representing a telescope/mount ASCOM driver.
//! @todo Support for multiple coordinate systems.
//! @todo Direction pad.
//! @ingroup plugin-devicecontrol
class AscomDeviceWidget : public QWidget
{
	Q_OBJECT
public:
	AscomDeviceWidget(TelescopeControl* plugin,
	                  const QString& id,
	                  const TelescopeClientAscomP& telescope,
	                  QWidget *parent = 0);
	
signals:
	
public slots:
	//! Slew the telescope to the manually entered coordinates.
	void slewToCoords();
	//! Slew the telescope to the view direction.
	void slewToCenter();
	//! Slew the telescope to the selected object.
	void slewToObject();
	
private:
	//! Pointer to the main plug-in class.
	TelescopeControl* deviceManager;
	
	//! Identifier of the controlled client.
	QString clientId;
	
	TelescopeClientAscomP client;
	
	CoordinatesWidget* coordsWidget;
	FovCirclesWidget* fcWidget;
};

#endif // ASCOMDEVICEWIDGET_HPP
