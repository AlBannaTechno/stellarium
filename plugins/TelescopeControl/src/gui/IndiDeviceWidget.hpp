/*
 * Qt-based INDI wire protocol client
 * 
 * Copyright (C) 2011 Bogdan Marinov
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
#include <QWidget>

#include "Indi.hpp"

//Control panel GUI elements:
#include "IndiGroupWidget.hpp"

class QTabWidget;

//! A representation of all available controls for a specific device.
//! A tab in the DeviceControlPanel window. Contains sub-tabs for the functional
//! groups. If no groups are specified, an artificial "Main" group/tab is used.
//! \author Bogdan Marinov
class IndiDeviceWidget : public QWidget
{
	Q_OBJECT

public:
	IndiDeviceWidget(const QString& deviceName, QWidget* parent = 0);

	void defineProperty(Property* property);
	void updateProperty(Property* property);
	void removeProperty(const QString& propertyName);
	//! Are there any properties defined in this device?
	bool isEmpty() const;

signals:
	void propertySet(const QString& deviceName,
	                 const QString& propertyName,
	                 const QVariantHash& elements);

private:
	QString deviceName;
	//! Contains a tab for each property group defined for the device.
	QTabWidget* groupsTabWidget;
	//! Groups are rendered as tabs inside the device tab.
	QHash<QString, IndiGroupWidget*> groupWidgets;
	//! Property widgets are handled on this level.
	QHash<QString, IndiPropertyWidget*> propertyWidgets;
};

#endif//_INDI_DEVICE_WIDGET_HPP_
