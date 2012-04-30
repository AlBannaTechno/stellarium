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
#include "StelStyle.hpp"

class Ui_StelDeviceWidget;
class TelescopeControl;

//! Control panel tab representing a TelescopeClient object.
//! \ingroup plugin-devicecontrol
class StelDeviceWidget : public QWidget
{
	Q_OBJECT
public:
	StelDeviceWidget(const QString& id,
	                 QWidget* parent = 0);
	virtual ~StelDeviceWidget();

public slots:
	void languageChanged();
	
private slots:
	//! reads the fields and slews a telescope
	void slew();

	//! sets the format of the input fields to Hours-Minutes-Seconds.
	//! Sets the right ascension field to HMS and the declination field to DMS.
	//! The parameter is necessary for signal/slot compatibility (QRadioButton).
	//! If "set" is "false", this method does nothing.
	void setFormatHMS(bool set);
	//! sets the format of the input fields to Degrees-Minutes-Seconds.
	//! Sets both the right ascension field and the declination field to DMS.
	//! The parameter is necessary for signal/slot compatibility (QRadioButton).
	//! If "set" is "false", this method does nothing.
	void setFormatDMS(bool set);
	//! sets the format of the input fields to Decimal degrees.
	//! Sets both the right ascension field and the declination field
	//! to decimal degrees.
	//! The parameter is necessary for signal/slot compatibility (QRadioButton).
	//! If "set" is "false", this method does nothing.
	void setFormatDecimal(bool set);

private:
	//! Pointer to the main plug-in class.
	TelescopeControl* deviceManager;
	
	//! Identifier of the controlled client.
	QString clientId;
	
	Ui_StelDeviceWidget* ui;
};

#endif // _SLEW_WINDOW_
