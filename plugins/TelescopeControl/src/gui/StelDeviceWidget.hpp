/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2010-2012 Bogdan Marinov (this file)
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
 
#ifndef _SLEW_WINDOW_HPP_
#define _SLEW_WINDOW_HPP_

#include <QObject>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "TelescopeClient.hpp"

class Ui_StelDeviceWidget;

class CoordinatesWidget;
class FovCirclesWidget;
class TelescopeControl;

//! Control panel tab representing a device controlled natively by Stellarium.
//! \ingroup plugin-devicecontrol
class StelDeviceWidget : public QWidget
{
	Q_OBJECT
public:
	StelDeviceWidget(TelescopeControl* plugin,
	                 const QString& id,
	                 const TelescopeClientP& telescope,
	                 QWidget* parent = 0);
	virtual ~StelDeviceWidget();

public slots:
	void retranslate();
	
private slots:
	//! Slew the telescope to the manually entered coordinates.
	void slewToCoords();
	//! Slew the telescope to the view direction.
	void slewToCenter();
	//! Slew the telescope to the selected object.
	void slewToObject();
	//! Set the input fields to current info
	void getCurrentObjectInfo();

private:
	//! Pointer to the main plug-in class.
	TelescopeControl* deviceManager;
	
	//! Identifier of the controlled client.
	QString clientId;
	
	TelescopeClientP client;
	
	Ui_StelDeviceWidget* ui;
	FovCirclesWidget* fcWidget;
	CoordinatesWidget* coordsWidget;
};

#endif // _SLEW_WINDOW_
