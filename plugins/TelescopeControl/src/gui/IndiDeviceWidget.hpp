/*
 * Qt-based INDI wire protocol client
 * 
 * Copyright (C) 2011, 2012 Bogdan Marinov
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _INDI_DEVICE_WIDGET_HPP_
#define _INDI_DEVICE_WIDGET_HPP_

#include <QHash>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QTabWidget>

#include "IndiDevice.hpp"
#include "IndiProperty.hpp"

//Control panel GUI elements:
#include "IndiGroupWidget.hpp"

//! A representation of all available controls for a specific device.
//! A tab in the DeviceControlPanel window. Contains sub-tabs for the functional
//! groups. If no groups are specified, an artificial "Main" group/tab is used.
//! \author Bogdan Marinov
class IndiDeviceWidget : public QTabWidget
{
	Q_OBJECT

public:
	IndiDeviceWidget(const DeviceP& newDevice, QWidget* parent = 0);
	
	//! Are there any properties defined in this device?
	bool isEmpty() const;
	
public slots:
	//! Adds a sub-widget of the appropriate type in the appropriate group tab.
	//! If no such group tab exists, a new one is created.
	void addProperty(const PropertyP& property);
	//! Removes the sub-widget of the property.
	//! If this was the last sub-widget in the group tab, it is also removed.
	void removeProperty(const QString& propertyName);

//signals:

private:
	//! \todo may become obsolete when I get rid of propertySet().
	QString deviceName;
	DeviceP device;
	
	//! Contains a tab for each property group defined for the device.
	// QTabWidget* groupsTabWidget; //TODO: Move the description if the change sticks.
	//! Groups are rendered as tabs inside the device tab.
	QHash<QString, IndiGroupWidget*> groupWidgets;
	//! Property widgets are handled on this level.
	QHash<QString, IndiPropertyWidget*> propertyWidgets;
};

#endif//_INDI_DEVICE_WIDGET_HPP_
